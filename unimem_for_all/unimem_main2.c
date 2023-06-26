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
#include <linux/inetdevice.h>
#include <linux/rtnetlink.h>
#include <linux/dma-direct.h>
#include <linux/utsname.h>

MODULE_LICENSE("GPL");

//#define PRINTFUNC() printk(KERN_ALERT CDEV_NAME ": %s called\n", __func__)


static struct pci_dev *pdev;
dma_addr_t dma_handle;
static void __iomem *mmio;
struct page *page = NULL;
u32 __iomem *kbuff, *kbufftemp;

unimem_data data;
acc_data * accel_data; //this is where the data passed on by the user are stored
unsigned long long int phys_address[24] = {0}; //this will only work for a maximum number of 16 remote allocations per app! #FIXME
unsigned long int virt_address[24] = {0};
int fpgadram_allocation_flag = 0; //this tells us whether the fpga memory region has been allocated
int hostdram_allocation_flag = 0;
int memory_allocations = 0;
int user_registers_vector[24]  = {0}; //these are registers that are given by the user app. Some might need translation
int user_registers_value[24]  = {0};
int acc_registers_vector[ACCELERATOR_COUNT][24] = {0}; //these are the registers that handle memory addresses and therefore, need translation, as shown by the registers.txt
int acc_registers_vector_size[ACCELERATOR_COUNT] = {0}; //this is the length for each of the registers_vector above

int tmp_accel_no;


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
static int network_mac, network_mac2;






#define ACC_REGISTER_HELPER _IOW('q', 'a', int *)
#define ACC_SETUP _IOW('q', 'b', acc_data *)
#define ACC_POLL _IOW('q', 'g', int *)
#define PASS_STRUCT _IOW('q', 'd', unimem_data *)



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



u32 get_default_ipaddr_by_devname(const char *devname)
{
        u32 addr;
        struct net_device *dev;
        if (!devname) return 0;
        /* find netdev by name, increment refcnt */
        dev= dev_get_by_name(&init_net, devname);
        if (!dev) return 0;
        /* get ip addr from rtable (global scope) */
        addr = inet_select_addr(dev, 0, RT_SCOPE_UNIVERSE);
        /* decrement netdev refcnt */
        dev_put(dev);
        return addr;
}



int unimemfile_open(struct inode* inode, struct file* filp)
{
   //  minor = MINOR(inode->i_rdev);

/*for_each_netdev(&init_net, netdev) {
    pr_info("Interface %s with MAC %02x:%02x:%02x:%02x:%02x:%02x\n"
            , netdev->name
            , netdev->dev_addr[0]
            , netdev->dev_addr[1]
            , netdev->dev_addr[2]
            , netdev->dev_addr[3]
            , netdev->dev_addr[4]
            , netdev->dev_addr[5]
        ); */
//}
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

  unsigned long int sum;
  sum = (GIGABYTE*0x3*data.node )+ (GIGABYTE*data.region);
  int ret=0;
  int replacement_candidate = 0;
  unsigned long pfn ;
   int i=0;
  //struct page *page = NULL;
  unsigned long len = 1024*1024*1024;
//  printk(KERN_NOTICE "MMAP!!\n");
//We need to set up 2 mmaps. One if the user asks for allocation on this node, and another if the user wants to access global shared memory.

  if(data.node == CURRENT_NODE){ //local mmap requested. 1)allcoate ram (host memory or FPGA resources), 2)set up the "from" bridge
   //printk(KERN_NOTICE "LOCAL(region0) MMAP!!\n"); //do 1GB kzalloc and assignt it to user, then configure the FROM.

          if(data.region == 0){ //allocate host DRAM

     // kbuff = dma_alloc_coherent(&pdev->dev, GIGABYTE, &dma_handle, GFP_KERNEL | GFP_DMA); //newly added GFP_DMA


//       if (kbuff == NULL) {
//         printk(KERN_NOTICE "LOCAL(dma-based) MMAP failed!!\n");
//         ret = -ENOMEM;
//         return ret;
//       }
   //    printk("Phys addr: 0x%lx\n", 0x100000000);

       pfn = (0x100000000 >> PAGE_SHIFT) + vma->vm_pgoff;

//     //  printk("dma_handle addr: 0x%x\n",i, dma_handle);
//       //the loop below checks that the physical memory is contiguous
// //      for (i=0;i<1024; i++){
//         kbufftemp = (virt_to_phys(kbuff));
//         printk("base CMA addr: 0x%x\n",kbufftemp);//}

//     printk("But the real addr is: 0x%lx\n\r",dma_to_phys(&pdev->dev, dma_handle));
//       //vma->vm_ops = &unimem_remap_vm_ops;

      vma->vm_flags |=  VM_DONTEXPAND | VM_DONTDUMP;
    //  set_memory_uc(kbuff, (len/PAGE_SIZE)); //set memory uncacheable
      //vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
      if (remap_pfn_range(vma, vma->vm_start, pfn, len, vma->vm_page_prot)){
        printk("remap page range failed\n");
        return -EAGAIN;
      }
      unimemFrom_replace_entry(info, UTLB,  0, 0x00000000, 0x00000000, /* this must be unimem now */0x0, /*0x80200430*/ 0x80200430, 0x28000000); // for address 0x40_0000_0000
          hostdram_allocation_flag = 1;

    }
else if(data.region == 1){ //1) allocate FPGA memory to the app and 2) set up the Unimem-FROM .. The address is the 0x150 for ddr4_0
//      vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
      vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
//      set_memory_wc((pci_resource_start(pdev, UNIMEM_BAR) + 0x400000000), (len/PAGE_SIZE)); //set memory uncacheable

      if (remap_pfn_range(vma, vma->vm_start, (pci_resource_start(pdev, UNIMEM_BAR) + 0x400000000) >> PAGE_SHIFT,
            vma->vm_end - vma->vm_start,
            vma->vm_page_prot ))
        return -EAGAIN;
      unimemFrom_replace_entry(info, UTLB,  1, 0x00000000, 0x00000000, /* this must be unimem now */0x0, 0x80200040, 0x28000000); // for address 0x40_0000_0000
    //unimemFROM set up!
    fpgadram_allocation_flag = 1;
     virt_address[memory_allocations] = data.base_addr;
      phys_address[memory_allocations] = 0x400000000;
     memory_allocations++;

          }
else if(data.region == 2){ //allocate accelerator 0
      vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
      if (io_remap_pfn_range(vma, vma->vm_start, (pci_resource_start(pdev, AXILITE_BAR) + 0xA0000) >> PAGE_SHIFT,
            0x1000,
            vma->vm_page_prot ))
        return -EAGAIN;
    }

else if(data.region == 3){ //1) allocate FPGA memory to the app and 2) set up the Unimem-FROM .. The address is the 0x150 for ddr4_0
//      vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
      vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
//      set_memory_wc((pci_resource_start(pdev, UNIMEM_BAR) + 0x400000000), (len/PAGE_SIZE)); //set memory uncacheable

      if (remap_pfn_range(vma, vma->vm_start, (pci_resource_start(pdev, UNIMEM_BAR) + 0xC00000000) >> PAGE_SHIFT,
            vma->vm_end - vma->vm_start,
            vma->vm_page_prot ))
        return -EAGAIN;
      unimemFrom_replace_entry(info, UTLB,  1, 0x00000000, 0x00000000, /* this must be unimem now */0x0, 0x80200010, 0x28000000); // for address 0x40_0000_0000
    //unimemFROM set up!
    fpgadram_allocation_flag = 1;
    virt_address[memory_allocations] = data.base_addr;
    phys_address[memory_allocations] = 0xC00000000;
    memory_allocations++;


          }

//     else if(data.region == 3){ //allocate accelerator 1
//       vma->vm_flags |=  VM_DONTEXPAND | VM_DONTDUMP;
//       set_memory_uc(kbuff, (len/PAGE_SIZE)); //set memory uncacheable

//       vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
//       if (io_remap_pfn_range(vma, vma->vm_start, (pci_resource_start(pdev, AXILITE_BAR) + 0xB0000) >> PAGE_SHIFT,
//             0x1000,
//             vma->vm_page_prot ))
//         return -EAGAIN;
//     }
//   else if(data.region == 4){ //allocate accelerator 2
//       vma->vm_flags |=  VM_DONTEXPAND | VM_DONTDUMP;
//       set_memory_uc(kbuff, (len/PAGE_SIZE)); //set memory uncacheable

//       vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
//       if (io_remap_pfn_range(vma, vma->vm_start, (pci_resource_start(pdev, AXILITE_BAR) + 0xC0000) >> PAGE_SHIFT,
//             0x1000,
//             vma->vm_page_prot ))
//         return -EAGAIN;
//     }
    //        printk("GVA alloc'd: %llx, size is 0x%x\n", UNIMEM_GVAS_BASE + GIGABYTE * (long long int)(data.region+data.node)), len;
            //UNIMEM-FROM MUST ALSO BE SET, but there is a problem with the slave bridge at the moment. #FIXME

      return 0;
    }

  else{ //remote "memory access" requested
    //printk(KERN_NOTICE "REMOTE MMAP!!\n");
    if(data.region == 0){ //allocate remote host DRAM

    //we are setting context=0 and coordinates == node 57 (0x1039)
     // replacement_candidate= unimemTo_replace(info, UTLB, /*0x02001039*/ 0x0 , /*(get_ip_coordinates(data.node)<<16) + 0x200*/ 0x405420, 0x0, 0x80201280, 0x18000000); // set the TO UTLB to the remote node
     // printk(KERN_NOTICE "Replacement candidate is: %d!!\n",replacement_candidate);
//      printk(KERN_NOTICE "Coordination bits is: %x!!\n", (get_ip_coordinates(data.node)<<16) + 0x200);
 //     printk(KERN_NOTICE "Replacement addr is: 0x%lx!!\n",GIGABYTE* (long long int)replacement_candidate);

      pfn = virt_to_phys(info->mem.start + 0x1800000000UL + sum) >> PAGE_SHIFT; //base address for the index
     // printk(KERN_NOTICE "Phys addr is: 0x%lx!!\n",virt_to_phys(info->mem.start + 0x1800000000 + (GIGABYTE*(long long int)replacement_candidate)));
           //printk(KERN_NOTICE "base Phys addr is: 0x%lx!!\n",virt_to_phys(info->mem.start ));

  ///   printk(KERN_NOTICE "base addr is: 0x%lx!!\n",info->mem.start + 0x180000000);
    //  printk(KERN_NOTICE "base addr + offset is: 0x%lx!!\n",(info->mem.start + 0x1800000000UL + sum));

      //vma->vm_ops = &unimem_remap_vm_ops;
      vma->vm_flags |= VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP;

     // vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

      if(io_remap_pfn_range(vma, vma->vm_start, (pci_resource_start(pdev, UNIMEM_BAR ) + 0x1800000000UL +  sum) >> PAGE_SHIFT, vma->vm_end - vma->vm_start,vma->vm_page_prot))
        return -EAGAIN;

//phys_address[memory_allocations] = 0x1800000000UL;
  //    printk(KERN_NOTICE "remote phys address before is: 0x%lx!!\n",phys_address[memory_allocations]);
// printk(KERN_NOTICE "nodeNo before is: 0x%lx!!\n",data.node);
// printk(KERN_NOTICE "region before is: 0x%lx!!\n",data.region);
      virt_address[memory_allocations] = data.base_addr;
      phys_address[memory_allocations] = 0x1800000000 + sum;
     

// printk(KERN_NOTICE "size of long int is: %d!!\n",sizeof(unsigned long int));
//printk(KERN_NOTICE "sum is: 0x%lx!!\n",sum);


      //printk(KERN_NOTICE "remote phys address after is: 0x%lx!!\n",phys_address[memory_allocations]);
      memory_allocations++;
      }
    else if(data.region == 1){ //allocate remote host FPGA memory
//      replacement_candidate= unimemTo_replace(info, UTLB, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ 0x405421, 0x0, 0x80201284, 0x18000000); // set the TO UTLB to the remote node -but point to the next entry
      pfn = virt_to_phys(info->mem.start + 0x1800000000UL + sum) >> PAGE_SHIFT; //base address for the index
     // vma->vm_ops = &unimem_remap_vm_ops;
      vma->vm_flags |= VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP;
       if(io_remap_pfn_range(vma, vma->vm_start, (pci_resource_start(pdev, UNIMEM_BAR ) + 0x1800000000UL + sum) >> PAGE_SHIFT, vma->vm_end - vma->vm_start,vma->vm_page_prot))
        return -EAGAIN;
      virt_address[memory_allocations] = data.base_addr;
      phys_address[memory_allocations] = 0x1800000000 + sum;
     memory_allocations++;
    }
  return 0;
  }
}


long unimem_ioctl( struct file *filep, unsigned int cmd, unsigned long arg){

int ret;
int i;
int accel_no;
switch(cmd){

    case PASS_STRUCT:
        ret = copy_from_user(&data, arg, sizeof(data));
        if(ret < 0){
            printk("PASS_STRUCT_ERROR\n");
            return -1;
        }

 //       printk("Message PASS_STRUCT node : %d\n",data.node);
 //       printk("Message PASS_STRUCT length : %d\n",data.length);
 //       printk("Message PASS_STRUCT region : %d\n",data.region);
 //       printk("Message PASS_STRUCT base_addr : %d\n",data.base_addr);

        break;

    case ACC_SETUP:
        ret =copy_from_user(accel_data, arg, sizeof(acc_data));
        if(ret < 0){
            printk("USE_ACCELERATOR ioctl error:\n");
            return -1;
        }
        printk("accel_no is: %d\n", accel_data->accel_number);

        ret = setup_accelerator(info, accel_data, acc_registers_vector, acc_registers_vector_size, phys_address, virt_address, memory_allocations);
         if(ret < 0)
                      printk("USE_ACCELERATOR ioctl error:\n");

    break;


    case ACC_POLL:
        ret =copy_from_user(accel_data, arg, sizeof(acc_data));
        if(ret < 0){
            printk("ACC_POLL ioctl error:\n");
            return -1;
        }
        accel_data->poll_value = poll_accel(info, accel_data->accel_number);
       // printk("Poll value:  %d\n", accel_data->poll_value);

        copy_to_user(arg, accel_data, sizeof(acc_data));
        break;

    case ACC_REGISTER_HELPER:

       ret = copy_from_user(&accel_no, arg, sizeof(int));
        if(ret < 0){
            printk("ACC_REGISTER_HELPER_ERROR\n");
            return -1;
        }

        ret =copy_from_user(&acc_registers_vector_size[accel_no], arg + sizeof(int), sizeof(int));
         if(ret < 0){
            printk("ACC_REGISTER_HELPER_ERROR\n");
            return -1;
        }

        ret = copy_from_user(&acc_registers_vector[accel_no], arg + sizeof(int)*2, sizeof(int)*acc_registers_vector_size[accel_no]*2+1);
        if(ret < 0){
            printk("ACC_REGISTER_HELPER_ERROR\n");
            return -1;
        }

        int x,y;
        for(x=0;x<ACCELERATOR_COUNT;x++){
                printk("accel_%d size is: %d\n", x, acc_registers_vector_size[x]);
                for(y=0;y<acc_registers_vector_size[x]*2;y++)
                {
                printk("vector[%d][%d]: %x", x , y , acc_registers_vector[x][y]);
                }
        }

      break;


    default :
        return -ENOTTY;
}

    return 0;
  }





int unimemfile_release (struct inode* inode, struct file* filp){
  int i = 0;
  int j = 0;
  dma_free_coherent(&pdev->dev, GIGABYTE, kbuff, dma_handle);
  fpgadram_allocation_flag = 0;
  hostdram_allocation_flag = 0;
  memory_allocations = 0;
  for (i=0;i<24;i++){
    phys_address[i] = 0;
    virt_address[i] = 0;
  }
    for (i=0;i<24;i++){
      user_registers_vector[i] = -1;
      user_registers_value[i] = -1;

    }
    for(i=0;i<ACCELERATOR_COUNT;i++){
        for(j=0;j<24;j++){
            acc_registers_vector[i][j] = 0;
        }
    }
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
  int err, i,j;
  struct pci_dev  *dev;
  struct in_device* in_dev;
  struct in_ifaddr* if_info;
  dev_t fdev;
  long int hex = 0x0;
  u32 __iomem *buffer; //a temp buffer variable to read/write
  u32 __iomem *bram_base; //a temp buffer variable to read/write
  u32 __iomem *to_apb_base; //a temp buffer variable to read/write
  u64 __iomem *unimem_base; //a temp buffer variable to read/write
  u64 __iomem *buffer2; //a temp buffer variable to read/write
printk("hostname: %s\n", utsname()->nodename);
if( strcmp(utsname()->nodename, "node21") == NULL ) // Outgoing packet must not have <NULL>
{
	hex=0x1621;
}	

else if( strcmp(utsname()->nodename, "node58") == NULL ) // Outgoing packet must not have <NULL>
{
        hex=0x1658;
}


else if( strcmp(utsname()->nodename, "node59") == NULL ) // Outgoing packet must not have <NULL>
{
        hex=0x1659;
}

else if( strcmp(utsname()->nodename, "node74") == NULL ) // Outgoing packet must not have <NULL>
{
        hex=0x1674;
}

else if( strcmp(utsname()->nodename, "node80") == NULL ) // Outgoing packet must not have <NULL>
{
        hex=0x1680;
}


//netdev = __dev_get_by_name(&init_net,"qdma-net"); //finding the network device to extract the MAC
  //if(!netdev) {
    //printk(KERN_ERR "Can't obtain device\n");
    //return;
//}
  //network_mac = netdev->dev_addr[5];
  //network_mac2 = netdev->dev_addr[4];
  //hex = network_mac2<<8;
  //hex += network_mac;
  //printk(KERN_ALERT "Network_mac LS is: %x \n",network_mac);
  //printk(KERN_ALERT "Network_mac MS is: %x \n",network_mac2);
  //printk(KERN_ALERT "Coordinates are : 0x%x \n",hex);



/*  netdev =  __dev_get_by_name(&init_net,"qdma-net"); //finding the network device to extract the MAC
  in_dev = (struct in_device *)netdev->ip_ptr;
  if_info = in_dev->ifa_list;
        for (;if_info;if_info=if_info->ifa_next)
                {
                if (!(strcmp(if_info->ifa_label,"qdma-net")))
                        {
                        printk("if_info->ifa_address=%d\n",if_info->ifa_address);
                        break;
                        }
                }
*/
/*struct net_device *netdev = dev_get_by_name(&init_net, "qdma-net");
if(!netdev) {
    printk(KERN_ERR "Can't obtain device\n");
    return;
}

// roughly
rcu_read_lock();
for(if_info = rcu_dereference(netdev->ip_ptr->ifa_list);
          if_info;
          if_info = rcu_dereference(if_info->ifa_next))
    printk("address: %pI4\n", &if_info->ifa_address);
    hex = get_default_ipaddr_by_devname("qdma-net");

  printk(KERN_ALERT "Network IP is: %dI4 \n",hex);

    rcu_read_unlock();
*/


  CURRENT_NODE = find_current_node(hex);
  printk(KERN_ALERT "Current node is: %d \n",CURRENT_NODE);

  info = kzalloc(sizeof(struct unimem_info), GFP_KERNEL);
  accel_data = kzalloc(sizeof(struct acc_data), GFP_KERNEL);
  printk(KERN_ALERT "Loading Unimem driver in kernel \n");

        if (pci_register_driver(&pci_driver) < 0) {
                        printk(KERN_ALERT "Error registering the driver\n");
                return 1;
        }
                        printk(KERN_ALERT "Driver registered\n");
//start = pci_resource_start(dev, BAR);

        dev = pci_get_device (UNIMEM_VENDOR_ID, UNIMEM_DEVICE_ID,NULL);
  pdev = dev;

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


        //printk(KERN_ALERT "Buffer addr before is %lx\n",        info->port.start);

//      info->port.start = memremap(start, pci_resource_len(dev, AXILITE_BAR),MEMREMAP_WT);
  //info->mem.start = memremap(start, pci_resource_len(dev, UNIMEM_BAR),MEMREMAP_WT);
  info->port.start = pci_iomap(dev, AXILITE_BAR, pci_resource_len(dev, AXILITE_BAR));
  info->mem.start = pci_iomap(dev, UNIMEM_BAR, pci_resource_len(dev, UNIMEM_BAR));
  //if (        info->port.start)
//printk(KERN_WARNING "Buffer addr is %lx\n",     info->mem.start);
bram_base = info->mem.start + 0x100000;
to_apb_base = info->port.start + 0x40000;

unimem_base = info->mem.start + 0x1800000000; //unimem_to //8 meta ton asso gia to UTLB
printk( "Unimem addr is %lx\n",         unimem_base);

bridge_set_BDF_entry(info, 0, 0x0);
printk( "Bram addr is %lx\n",   bram_base);

for(i=0;i<ACCELERATOR_COUNT;i++){
    for(j=0;j<24;j++){
        acc_registers_vector[i][j] = 0;
    }
}
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


unimemTo_replace_entry(info, UTLB, 0, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x405420, 0x0, 0x80201280, 0x28000000); //node21
unimemTo_replace_entry(info, UTLB, 1, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x405421, 0x0, 0x80201284, 0x28000000);
unimemTo_replace_entry(info, UTLB, 2, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x405422, 0x0, 0x80201288, 0x28000000);
unimemTo_replace_entry(info, UTLB, 3, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x40E820, 0x0, 0x80201280, 0x28000000); //node58
unimemTo_replace_entry(info, UTLB, 4, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x40E821, 0x0, 0x80201284, 0x28000000);
unimemTo_replace_entry(info, UTLB, 5, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x40E822, 0x0, 0x80201288, 0x28000000); 
unimemTo_replace_entry(info, UTLB, 6, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x40EC20, 0x0, 0x80201280, 0x28000000); //node59
unimemTo_replace_entry(info, UTLB, 7, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x40EC21, 0x0, 0x80201284, 0x28000000);
unimemTo_replace_entry(info, UTLB, 8, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x40EC22, 0x0, 0x80201288, 0x28000000);
unimemTo_replace_entry(info, UTLB, 9, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x412820, 0x0, 0x80201280, 0x28000000); //node74
unimemTo_replace_entry(info, UTLB, 10, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x412821, 0x0, 0x80201284, 0x28000000);
unimemTo_replace_entry(info, UTLB, 11, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x412822, 0x0, 0x80201288, 0x28000000);
unimemTo_replace_entry(info, UTLB, 12, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x414020, 0x0, 0x80201280, 0x28000000); //node80
unimemTo_replace_entry(info, UTLB, 13, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x414021, 0x0, 0x80201284, 0x28000000);
unimemTo_replace_entry(info, UTLB, 14, 0x0, /*(get_ip_coordinates(data.node)<<16) + 0x240*/ /*0x40EC20*/0x414022, 0x0, 0x80201288, 0x28000000); 




unimemFrom_replace_entry(info, UTLB,  0, 0x00000000, 0x00000000, /* this must be unimem now */0x0, 0x80200430, 0x28000000);
unimemFrom_replace_entry(info, UTLB,  1, 0x00000000, 0x00000000, /* this must be unimem now */0x0, 0x80200040, 0x28000000); // for address 0x40_0000_0000
unimemFrom_replace_entry(info, UTLB,  2, 0x00000000, 0x00000000, /* this must be unimem now */0x0, 0x802000C0, 0x28000000); // for address 0x40_0000_0000


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
      device_create(mychardev_class, NULL, MKDEV(dev_major, i), NULL, "unimem_file");
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



