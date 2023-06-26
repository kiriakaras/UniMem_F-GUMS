#ifndef UNIMEMDEFS_H
#define UNIMEMDEFS_H


#define MAX_DEV 1
#define ACCELERATOR_COUNT 8

#define NODE_0 4e

#define UTLB 0
#define PTLB 1

#define GIGABYTE 0x40000000UL
#define MEGABYTE 0x40000UL

#define DMA_BAR 0 //typical address is 0x3e380000000
#define ACCELERATOR_BAR 1
#define AXILITE_BAR 2 //typical address is 0x3e000000000
#define UNIMEM_BAR 4 //typical address is 0x3c000000000

#define MAX_UNIMEM_MAPS 1
#define MAX_UNIMEM_PORT_REGIONS 0

#define UNIMEM_DEVICE_ID 0x933f
#define UNIMEM_VENDOR_ID 0x10ee
#define UNIMEM_TO_APB_OFFSET 0x40000
#define UNIMEM_FROM_APB_OFFSET 0x30000
#define BRIDGE_CSR_OFFSET 0x20000000 //PCI slave bridge

#define BRIDGE_CSR_BDF0_BASE 0x20002420 //BDF0 base

#define BRIDGE_CSR_BDF0_REG0 0x20002420 //BDF0 reg0
#define BRIDGE_CSR_BDF0_REG1 0x20002424 //BDF0 reg1
#define BRIDGE_CSR_BDF0_REG2 0x20002428 //BDF0 reg2
#define BRIDGE_CSR_BDF0_REG3 0x2000242C //BDF0 reg3
#define BRIDGE_CSR_BDF0_REG4 0x20002430 //BDF0 reg4
#define BRIDGE_CSR_BDF0_REG5 0x20002434 //BDF0 reg5



#define UNIMEM_GVAS_BASE 0x10000000000

#define FPGA_DDR4_0 0x50000000000
#define FPGA_DDR4_2 0x58000000000


#define NODE_0_MAC 0x10
#define NODE_1_MAC 0x50 //this needs to be changed to something that exists




#define PRINTFUNC() printk(KERN_ALERT CDEV_NAME ": %s called\n", __func__)

extern int accelerators_base[];


typedef struct unimem_mem{
    const char *name;
    void __iomem *start;
    unsigned long size;
}unimem_mem ;

typedef struct unimem_io {
    const char *name;
    unsigned long start;
    unsigned long size;
}unimem_io;

typedef struct mmap_info {
        char *data;     /* the data */
        int vmas;       /* how many times it is mmapped */
}mmap_info;


typedef struct unimem_info {
    struct unimem_mem mem;
    struct unimem_io port;
    u8 byte; //the IRQ functionality will be added later #FIXME
    u16 word;
    u32 dword;
}unimem_info;


typedef struct unimem_data {
    int region;
    int length;
    int node;
    unsigned long int base_addr;
    } unimem_data;

typedef struct acc_data {
    int accel_number; //which is the requested accelerator?
    int reg_number; //should be less than 16
    int reg_offset[24]; // the register offsets to be written
    long int reg_data[24];       //and their data
    volatile int poll_value;
    } acc_data;


#endif

