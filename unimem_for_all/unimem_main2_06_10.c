#include <linux/cdev.h> /* cdev_ */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/uaccess.h> /* put_user */
#include <linux/ioctl.h>
#include <linux/hugetlb.h>
#include "unimem_defs.h"
#include "unimem_ops.h"
#include <linux/netdevice.h>
#include <linux/dma-mapping.h>
#include <linux/set_memory.h>
#include <linux/dma-direct.h>


MODULE_LICENSE("GPL");

//#define PRINTFUNC() printk(KERN_ALERT CDEV_NAME ": %s called\n", __func__)


static struct pci_dev *pdev;
dma_addr_t dma_handle;
static void __iomem *mmio;
struct page *page = NULL;
u32 __iomem *kbuff, *kbufftemp;

unimem_data data;

// 2-level TLBs. 16 entries, with valid bit. If valid bit == 1, second level TLBs are accessed, that contain the VA to PA translation pairs

static unsigned int utlb[16][2];
static unsigned int ptlb[16][2];

//16 entries, each row contains the PA, data0 to data4.
static unsigned int utlb_PA_entries[16][5];
//16 entries, each row contains the VA address
static unsigned long int utlb_VA_entries[16][1];

//16 entries, each row contains the PA, data0 to data4.
static unsigned int ptlb_PA_entries[16][5];
//16 entries, each row contains the VA address
static unsigned long int ptlb_VA_entries[16][1];



struct mychar_device_data {
    struct cdev cdev;
};

// sysfs class structure
static struct class *mychardev_class = NULL;

// array of mychar_device_data for
static struct mychar_device_data mychardev_data[MAX_DEV];


static int dev_major = 0;
static int major;
static int minor;
static int network_mac;






#define PASS_STRUCT _IOW('q', 'a', unimem_data *)

static const struct pci_device_id pci_ids[] = {
        { PCI_DEVICE(0x10ee, 0x933f), },        /** PF 3 */
        { 0, }
};
MODULE_DEVICE_TABLE(pci, pci_ids);


struct unimem_info *info;


int CURRENT_NODE;
struct net_device *netdev;

static int unimem_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
  printk(KERN_ALERT "Probe is up.. \n");
  return 0;
}


static void unimem_remove(struct pci_dev *dev)
{
  printk(KERN_ALERT "Removing.. \n");
}



static struct pci_driver pci_driver = {
        .name     = "unimem_driver",
        .id_table = pci_ids,
        .probe    = unimem_probe,
        .remove   = unimem_remove,
};



void unimem_vma_open(struct vm_area_struct *vma)
{
    printk(KERN_NOTICE "Simple VMA open, virt %lx, phys ");
}

void unimem_vma_close(struct vm_area_struct *vma)
{
    printk(KERN_NOTICE "Simple VMA close.\n");
}



vm_fault_t unimem_vma_fault( struct vm_fault* vmf)
{
    // struct page* page;
    // struct mmap_info* info;
    // unsigned long address;
        // address = (unsigned long)vmf->address;
    // PRINTFUNC();

    /* is the address valid? */
    // if (address > vma->vm_end) {
    // printk(KERN_NOTICE "Invalid address!!\n");

    //     return VM_FAULT_SIGBUS;
    // }

    // /* the data is in vma->vm_private_data */
    // info = (struct mmap_info*)vma->vm_private_data;
    // if (!info->data) {
     printk(KERN_NOTICE "No data!!\n");
         return VM_FAULT_SIGBUS;
    // }

    // /* get the page */
    // page = virt_to_page(info->data);

    // /* increment the reference count of this page */
    // get_page(page);
    // /* type is the page fault type */
    // vmf->page = page;
    return 0;
}

static struct vm_operations_struct unimem_remap_vm_ops = {
    .open =  unimem_vma_open,
    .close = unimem_vma_close,
        .fault = unimem_vma_fault,
};


int unimemfile_open(struct inode* inode, struct file* filp)
{
   //  minor = MINOR(inode->i_rdev);

for_each_netdev(&init_net, netdev) {
    pr_info("Interface %s with MAC %02x:%02x:%02x:%02x:%02x:%02x\n"
            , netdev->name
            , netdev->dev_addr[0]
            , netdev->dev_addr[1]
            , netdev->dev_addr[2]
            , netdev->dev_addr[3]
            , netdev->dev_addr[4]
            , netdev->dev_addr[5]
        );
}
  //dev->dev_addr; // is the MAC address
 // pr_info("Interface qdma-net with MAC %02x\n", netdev->dev_addr[5]);
//pr_info("Interface qdma-net with MAC %x\n", dev->dev_addr[5]);
/*
     printk(KERN_NOTICE "OPEN minor: %d\n", minor);
       printk(KERN_ALERT "CURR_NODE IS %d \n",CURRENT_NODE); // if nodeNo == current_node, kzalloc and set the from, otherwise remote alloc.

     if(minor == CURRENT_NODE)
  printk(KERN_ALERT "Same node, unimem file opened, need to check the nodeNo... \n"); // if nodeNo == current_node, kzalloc and set the from, otherwise remote alloc.
  else {
    printk(KERN_ALERT "Different node, cannot open! need to check the nodeNo... \n"); // if nodeNo == current_node, kzalloc and set the from, otherwise remote alloc.
    return -1;
  }
*/
    return 0;
}


int unimemfile_mmap(struct file * filp, struct vm_area_struct * vma){

  int ret=0;
  int replacement_candidate = 0;
  unsigned long pfn ;
   int i=0;
  //struct page *page = NULL;
  unsigned long len = 1024*1024*1024;
  printk(KERN_NOTICE "MMAP!!\n");
//We need to set up 2 mmaps. One if the user asks for allocation on this node, and another if the user wants to access global shared memory.

  if(data.node == CURRENT_NODE){ //local mmap requested. 1)allcoate ram (host memory or FPGA resources), 2)set up the "from" bridge
    if(data.region == 0){ //allocate host DRAM
      printk(KERN_NOTICE "LOCAL(region0) MMAP!!\n"); //do 1GB kzalloc and assignt it to user, then configure the FROM.

     // kbuff = dma_alloc_coherent(&pdev->dev, GIGABYTE, &dma_handle, GFP_KERNEL | GFP_DMA); //newly added GFP_DMA

     
//       if (kbuff == NULL) {
//         printk(KERN_NOTICE "LOCAL(dma-based) MMAP failed!!\n");
//         ret = -ENOMEM;
//         return ret;
//       }
       printk("Phys addr: 0x%lx\n", 0x100000000);

       pfn = (0x100000000 >> PAGE_SHIFT) + vma->vm_pgoff;

//     //  printk("dma_handle addr: 0x%x\n",i, dma_handle);
//       //the loop below checks that the physical memory is contiguous
// //      for (i=0;i<1024; i++){
//         kbufftemp = (virt_to_phys(kbuff));
//         printk("base CMA addr: 0x%x\n",kbufftemp);//}

//     printk("But the real addr is: 0x%lx\n\r",dma_to_phys(&pdev->dev, dma_handle));
//       //vma->vm_ops = &unimem_remap_vm_ops;

      vma->vm_flags |=  VM_DONTEXPAND | VM_DONTDUMP;
      printk("vm_offset: %ld\n",vma->vm_pgoff);
    //  set_memory_uc(kbuff, (len/PAGE_SIZE)); //set memory uncacheable
      vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
      if (remap_pfn_range(vma, vma->vm_start, pfn, len, vma->vm_page_prot)){
        printk("remap page range failed\n");
        return -EAGAIN;
      }
    }
else if(data.region == 1){ //1) allocate FPGA memory to the app and 2) set up the Unimem-FROM .. The address is the 0x150 for ddr4_0
      vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
      if (io_remap_pfn_range(vma, vma->vm_start, (pci_resource_start(pdev, UNIMEM_BAR) + 0x400000000) >> PAGE_SHIFT,
            vma->vm_end - vma->vm_start,
            vma->vm_page_prot ))
        return -EAGAIN;
      unimemFrom_replace_entry(info, UTLB,  1, 0x00000000, 0x00000000, /* this must be unimem now */0x0, 0x80200040, 0x00000000); // for address 0x40_0000_0000
    //unimemFROM set up!
          }
      else if(data.region == 2){ //allocate host DRAM with dma 
      printk(KERN_NOTICE "LOCAL(CMA) MMAP!!\n"); //do 1GB kzalloc and assignt it to user, then configure the FROM.
            printk(KERN_NOTICE "LOCAL(CMA) MMAPv2!!\n"); //do 1GB kzalloc and assignt it to user, then configure the FROM.
          if (!dma_declare_coherent_memory(&pdev->dev, 0x100000000,
			    0x100000000, GIGABYTE, DMA_ATTR_FORCE_CONTIGUOUS)) {
        printk(KERN_ALERT "declare ok\n");
    }
      kbuff = dma_alloc_coherent(&pdev->dev, GIGABYTE, &dma_handle, GFP_KERNEL); //newly added GFP_DMA
      if (kbuff == NULL) {
        printk(KERN_NOTICE "LOCAL(dma-based) MMAP failed!!\n");
        ret = -ENOMEM;
        return ret;
      }
      pfn = (virt_to_phys(kbuff) >> PAGE_SHIFT) + vma->vm_pgoff;

  //  printk("dma_handle addr: 0x%x\n",i, dma_handle);
    //the loop below checks that the physical memory is contiguous
//      for (i=0;i<1024; i++){
      kbufftemp = (virt_to_phys(kbuff));
      printk("base CMA addr: 0x%lx\n",kbufftemp);//}

      printk("But the real addr is: 0x%lx\n\r",dma_to_phys(&pdev->dev, dma_handle) >> PAGE_SHIFT);
      //vma->vm_ops = &unimem_remap_vm_ops;

      vma->vm_flags |=  VM_DONTEXPAND | VM_DONTDUMP;
      printk("vm_offset: %ld\n",vma->vm_pgoff);
      set_memory_uc(kbuff, (len/PAGE_SIZE)); //set memory uncacheable

      vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
      if (remap_pfn_range(vma, vma->vm_start, pfn, len, vma->vm_page_prot)){
        printk("remap page range failed\n");
        return -EAGAIN;
      }
    }
            printk("GVA alloc'd: %llx, size is 0x%x\n", UNIMEM_GVAS_BASE + GIGABYTE * (long long int)(data.region+data.node)), len;
            //UNIMEM-FROM MUST ALSO BE SET, but there is a problem with the slave bridge at the moment. #FIXME

      return 0;
    }

  else{ //remote "memory access" requested
    printk(KERN_NOTICE "REMOTE MMAP!!\n");
    if(data.region == 0){ //allocate remote host DRAM
      replacement_candidate= unimemTo_replace(info, UTLB, 0x0, 0x0, 0x0, 0x80201280, 0x0); // set the TO UTLB to the remote node
      printk(KERN_NOTICE "Replacement candidate is: %d!!\n",replacement_candidate);
      printk(KERN_NOTICE "Replacement addr is: 0x%lx!!\n",GIGABYTE* (long long int)replacement_candidate);

      pfn = virt_to_phys(info->mem.start + 0x1800000000 + (GIGABYTE*(long long int)replacement_candidate)) >> PAGE_SHIFT; //base address for the index
      printk(KERN_NOTICE "Phys addr is: 0x%lx!!\n",virt_to_phys(info->mem.start + 0x1800000000 + (GIGABYTE*(long long int)replacement_candidate)));
            printk(KERN_NOTICE "base Phys addr is: 0x%lx!!\n",virt_to_phys(info->mem.start ));

      printk(KERN_NOTICE "base addr is: 0x%lx!!\n",info->mem.start + 0x180000000);
      printk(KERN_NOTICE "base addr + offset is: 0x%lx!!\n",(info->mem.start + 0x1800000000 + (GIGABYTE*(long long int)replacement_candidate)));

      //vma->vm_ops = &unimem_remap_vm_ops;
      vma->vm_flags |= VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP;

      vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

      if(io_remap_pfn_range(vma, vma->vm_start, (pci_resource_start(pdev, UNIMEM_BAR ) + 0x1800000000 + (GIGABYTE*(long long int)replacement_candidate)) >> PAGE_SHIFT, vma->vm_end - vma->vm_start,vma->vm_page_prot))
        return -EAGAIN;
      }
    else if(data.region == 1){ //allocate remote host FPGA memory
      replacement_candidate= unimemTo_replace(info, UTLB, 0x0, 0x0, 0x0, 0x80201284, 0x0); // set the TO UTLB to the remote node -but point to the next entry
      pfn = virt_to_phys(info->mem.start + 0x1800000000 + (GIGABYTE*(long long int)replacement_candidate)) >> PAGE_SHIFT; //base address for the index
     // vma->vm_ops = &unimem_remap_vm_ops;
      vma->vm_flags |= VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP;
       if(io_remap_pfn_range(vma, vma->vm_start, (pci_resource_start(pdev, UNIMEM_BAR ) + 0x1800000000 + (GIGABYTE*(long long int)replacement_candidate)) >> PAGE_SHIFT, vma->vm_end - vma->vm_start,vma->vm_page_prot))
        return -EAGAIN;
    }
  return 0;
  }
}


long unimem_ioctl( struct file *filep, unsigned int cmd, unsigned long arg){

int ret;


switch(cmd){

    case PASS_STRUCT:
        ret = copy_from_user(&data, arg, sizeof(data));
        if(ret < 0){
            printk("PASS_STRUCT_ERROR\n");
            return -1;
        }

        printk("Message PASS_STRUCT node : %d\n",data.node);
        printk("Message PASS_STRUCT length : %d\n",data.length);
        printk("Message PASS_STRUCT region : %d\n",data.region);
        printk("Message PASS_STRUCT base_addr : %d\n",data.base_addr);

        break;

    default :
        return -ENOTTY;
}

return 0;
  }





int unimemfile_release (struct inode* inode, struct file* filp){
  dma_free_coherent(&pdev->dev, GIGABYTE, kbuff, dma_handle);
  printk(KERN_NOTICE "released!!\n");
  return 0;
}





static const struct file_operations unimem_fops = {
        .open = unimemfile_open,
  .release = unimemfile_release,
        .mmap = unimemfile_mmap,
  .unlocked_ioctl = unimem_ioctl,

};


static int unimem_init(void)
{

  int candidate = 0;
  resource_size_t start;
  resource_size_t end;
  int err, i;
  struct pci_dev  *dev;
  dev_t fdev;
  u32 __iomem *buffer; //a temp buffer variable to read/write
  u32 __iomem *bram_base; //a temp buffer variable to read/write
  u32 __iomem *to_apb_base; //a temp buffer variable to read/write
  u64 __iomem *unimem_base; //a temp buffer variable to read/write
  u64 __iomem *buffer2; //a temp buffer variable to read/write
  netdev = __dev_get_by_name(&init_net,"enp130s0f1"); //finding the network device to extract the MAC
  network_mac = netdev->dev_addr[5];
  printk(KERN_ALERT "Network_mac is: %x \n",network_mac);

  CURRENT_NODE = find_current_node(network_mac);

  info = kzalloc(sizeof(struct unimem_info), GFP_KERNEL);


  printk(KERN_ALERT "Loading Unimem driver in kernel \n");

        if (pci_register_driver(&pci_driver) < 0) {
                        printk(KERN_ALERT "Error registering the driver\n");
                return 1;
        }
                        printk(KERN_ALERT "Driver registered\n");
//start = pci_resource_start(dev, BAR);

        dev = pci_get_device (UNIMEM_VENDOR_ID, UNIMEM_DEVICE_ID,NULL);
  pdev = dev;
        if (dev== NULL)
          printk(KERN_ALERT "PCI device not found! \n");


if (!dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64))) {
  printk(KERN_ALERT "DMA_64_MASK_OK\n");

}

        start = pci_resource_start(dev, AXILITE_BAR);
        printk("\nAXI Lite Start is %lx\n", start);
  printk("AXI Lite End is %lx\n", pci_resource_end(dev, AXILITE_BAR));
  printk("AXI Lite Length is 0x%lx\n\n",  pci_resource_end(dev, AXILITE_BAR) - pci_resource_start(dev, AXILITE_BAR) +1);

  printk("Unimem Start is %lx\n", start);
  printk("Unimem End is %lx\n", pci_resource_end(dev, UNIMEM_BAR));
  printk("Unimem Length is 0x%lx\n\n",  pci_resource_end(dev, UNIMEM_BAR) - pci_resource_start(dev, UNIMEM_BAR) +1);


        printk(KERN_ALERT "Buffer addr before is %lx\n",        info->port.start);

//      info->port.start = memremap(start, pci_resource_len(dev, AXILITE_BAR),MEMREMAP_WT);
  //info->mem.start = memremap(start, pci_resource_len(dev, UNIMEM_BAR),MEMREMAP_WT);
  info->port.start = pci_iomap(dev, AXILITE_BAR, pci_resource_len(dev, AXILITE_BAR));
  info->mem.start = pci_iomap(dev, UNIMEM_BAR, pci_resource_len(dev, UNIMEM_BAR));
  //if (        info->port.start)
printk(KERN_WARNING "Buffer addr is %lx\n",     info->mem.start);
bram_base = info->mem.start + 0x100000;
to_apb_base = info->port.start + 0x40000;

unimem_base = info->mem.start + 0x1800000000; //unimem_to //8 meta ton asso gia to UTLB
printk( "Unimem addr is %lx\n",         unimem_base);

bridge_set_BDF_entry(info, 0, 0x0);
printk( "Bram addr is %lx\n",   bram_base);


unimemTo_enable(info);
unimemFrom_enable(info);
bridge_read_BDF_entry(info, 0);
// buffer = bram_base;

//      printk(KERN_ALERT "Buffer[0] before is %lx\n",  buffer[0]);
//      printk(KERN_ALERT "Buffer[1] before is %lx\n",  buffer[1]);
//      printk(KERN_ALERT "Buffer[2] before is %lx\n",  buffer[2]);

// buffer[0] = 0xfefe;

// buffer[1] = 0xbeef;
// buffer[2] = 0xfefecaca;
//      printk(KERN_ALERT "Buffer[0] after is %lx\n",   buffer[0]);
//      printk(KERN_ALERT "Buffer[1] after is %lx\n",   buffer[1]);
//      printk(KERN_ALERT "Buffer[2] after is %lx\n",   buffer[2]);

//unimem_find_eviction_candidate(info, UTLB);

//candidate = unimemTo_replace_entry(info, UTLB,  0, 0x00000000, 0x00000000, /* this must be unimem now */0x01000000, 0x80200000, 0x00000000);

//for maxilink--to get across and reach the base addr of uFrom, and the FROM to point to mig.
candidate = unimemTo_replace_entry(info, UTLB,  0, 0x00000000, 0x00000000, /* this must be unimem now */0x0, 0x80201500, 0x00000000); // for address 0x28_0000_0000, UTLB of FROM.(32GB offset)
candidate = unimemFrom_replace_entry(info, UTLB,  0, 0x00000000, 0x00000000, /* this must be unimem now */0x0, 0x80200430, 0x00000000); // for address 0x40_0000_0000
/////

//candidate = unimemTo_replace_entry(info, UTLB,  0, 0x00000000, 0x00000000, /* this must be unimem now */0x0, 0x80200280, 0x00000000); // for address 0x28_0000_0000, UTLB of FROM.(32GB offset)

//candidate = unimemTo_replace_entry(info, UTLB,  0, 0x00000000, 0x00000000, /* this must be unimem now */0x0, 0x80200400, 0x00000000); // for address 0x40_0000_0000
//candidate = unimemFrom_replace_entry(info, UTLB,  0, 0x00000000, 0x00000000, /* this must be unimem now */0x0, 0x80200400, 0x00000000); // for address 0x40_0000_0000


//candidate = unimemFrom_replace_entry(info, UTLB,  0, 0x00000000, 0x00000000, /* BRAM address */0x01000000, 0x80200000, 0x00000000);


// //candidate = unimem_replace_entry(info, UTLB, 0, 0, 0x00000000, 0x00000000, 0x80200540, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB, 1, 1, 0x00000000, 0x00000000, 0x80200540, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB, 2, 2, 0x00000000, 0x00000000, 0x80200540, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB, 3, 3, 0x00000000, 0x00000000, 0x80200540, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB, 4, 4, 0x00000000, 0x00000000, 0x80200540, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB,  5, 5, 0x00000000, 0x00000000, 0x80200500, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB,  6, 6, 0x00000000, 0x00000000, 0x80200500, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB,  7, 7, 0x00000000, 0x00000000, 0x80200500, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB,  8, 8, 0x00000000, 0x00000000, 0x80200500, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB,  9, 9, 0x00000000, 0x00000000, 0x80200500, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB,  10, 10, 0x00000000, 0x00000000, 0x80200500, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB,  11, 11, 0x00000000, 0x00000000, 0x80200500, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB,  12, 12, 0x00000000, 0x00000000, 0x80200500, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB,  13, 13, 0x00000000, 0x00000000, 0x80200500, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB,  14, 14, 0x00000000, 0x00000000, 0x80200500, 0x00000000);
// candidate = unimemTo_replace_entry(info, UTLB,  15, 15, 0x00000000, 0x00000000, 0x80200500, 0x00000000);
// //candidate = unimem_replace_entry(info, UTLB,  16, 16, 0x00000000, 0x00000000, 0x80200500, 0x00000000);



//buffer2 = unimem_base;

//      printk(KERN_ALERT "Buffer_ddr[0] before is %lx\n",      buffer2[0]);
//      printk(KERN_ALERT "Buffer_ddr[1] before is %lx\n",      buffer2[1]);
//      printk(KERN_ALERT "Buffer_ddr[20000] before is %lx\n",  buffer2[20000]);
//buffer2[0] = 0xfefefefe1;

//buffer2[1] = 0xbeefbeef2;
//buffer2[2] = 0xfefecacaa3;
//buffer2[20000] = 0xfefecacaa3;

//      printk(KERN_ALERT "Buffer_ddr[0] after is %lx\n",       buffer2[0]);
//      printk(KERN_ALERT "Buffer_ddr[1] after is %lx\n",       buffer2[1]);
//      printk(KERN_ALERT "Buffer_ddr[20000] after is %lx\n",   buffer2[20000]);
//      printk(KERN_ALERT "Replaced candidate was %d\n",        candidate);


//candidate = unimem_replace(info, UTLB, 0x12, 0x22222, 0x376876, 0x4877, 0x5345);
//candidate = unimem_replace(info, UTLB, 0x32, 0x200, 0x3, 0x4789, 0x5657);
//candidate = unimem_replace(info, UTLB, 0x55, 0x20, 0x323423, 0x48980, 0x5567);
//candidate = unimem_replace(info, UTLB, 0x52, 0x20000000, 0x3, 0x409, 0x5567);


//lets create a file from here on//

    // allocate chardev region and assign Major number
    err = alloc_chrdev_region(&fdev, 0, MAX_DEV, "unimem_file");

    dev_major = MAJOR(fdev);

    // create sysfs class
    mychardev_class = class_create(THIS_MODULE, "unimem_file");

    // Create necessary number of the devices
    for (i = 0; i < MAX_DEV; i++) {
        // init new device
        cdev_init(&mychardev_data[i].cdev, &unimem_fops);
        mychardev_data[i].cdev.owner = THIS_MODULE;

        // add device to the system where "i" is a Minor number of the new device
        cdev_add(&mychardev_data[i].cdev, MKDEV(dev_major, i), 1);

        // create device node /dev/mychardev-x where "x" is "i", equal to the Minor number
        device_create(mychardev_class, NULL, MKDEV(dev_major, i), NULL, "unimem_file-%d", i);
        }

  return 0;
}

static void unimem_exit(void)
{
        int i=0;
        printk(KERN_ALERT "Removing Unimem driver from kernel\n");
 pci_iounmap(pdev, info->port.start);
 //memunmap(info->mem.start);
pci_iounmap(pdev, info->mem.start);
        printk(KERN_ALERT "Unmapped\n");

    for (i = 0; i < MAX_DEV; i++) {
        device_destroy(mychardev_class, MKDEV(dev_major, i));
    }

//      class_unregister(mychardev_class);
//    class_destroy(mychardev_class);
        pci_unregister_driver(&pci_driver);
//      unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);
//      debugfs_remove(file1);
    for (i = 0; i < MAX_DEV; i++) {
        device_destroy(mychardev_class, MKDEV(dev_major, i));
    }

        class_unregister(mychardev_class);
    class_destroy(mychardev_class);
        unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);

}

module_init(unimem_init);
module_exit(unimem_exit);


