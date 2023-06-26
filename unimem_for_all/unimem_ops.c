#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <linux/pci.h>
#include <linux/uaccess.h> /* put_user */
#include <linux/ioctl.h>
#include "unimem_ops.h"
#include "unimem_defs.h"


extern int accelerators_base[] = {0xD0000, 0xE0000, 0xF0000, 0x80000, 0x90000, 0xA0000, 0xB0000, 0xC0000};

int mac_address[32] = { 0x1621, 0x1658, 0x1659, 0x1674, 0x1680, 0x5a, 0x60, 0x70,
        0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x100, 0x110, 0x120,
        0x130, 0x140, 0x150, 0x160, 0x170, 0x180, 0x190, 0x1a0, 0x1b0, 0x1c0, 0x1d0, 0x1e0, 0x1f0 };


int ip_address[32] = { 0x1015, 0x103A, 0x103B, 0x104A, 0x1050, 0x5a, 0x60, 0x70,
        0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x100, 0x110, 0x120,
        0x130, 0x140, 0x150, 0x160, 0x170, 0x180, 0x190, 0x1a0, 0x1b0, 0x1c0, 0x1d0, 0x1e0, 0x1f0 };

//Set up the operation register to "find eviction candidate" and read the index register. Return the value.

int find_current_node(int address){
        int i;

        for(i=0; i<32; i++)

                if(address == mac_address[i]){

                        return i;
                }
        return -1;
}

int find_ip_coordinates(int address){
        int i;

        for(i=0; i<32; i++)

                if(address == mac_address[i]){

                        return ip_address[i];
                }
        return -1;
}

int get_ip_coordinates(int index){
        return ip_address[index];
}


 //Ideally, this function should one of the bridge entries to point to the base physical address returned by dma_alloc_coherent().
 //For testing, only 1 region will be used
int bridge_set_BDF_entry(struct unimem_info * info, int BDF_entry, unsigned int phys_addr){ //and return the previous index so that the TLB can be updated
        int i;
        volatile u32 __iomem * bridge_csr_base; //a temp buffer variable to read/write
//      printk(KERN_ALERT "Configuring the CSR..\n");
        bridge_csr_base = info->port.start + BRIDGE_CSR_BDF0_BASE;
        for(i=0;i<8;i++){
                bridge_csr_base[0 + (i*8)]= 0x0;
                bridge_csr_base[1 + (i*8)]= 0x0;
                bridge_csr_base[2 + (i*8)]= 0x0;
                bridge_csr_base[3 + (i*8)]= 0x0;
                bridge_csr_base[4 + (i*8)]= 0xC0200000;//set to 8GB , fingers crossed;
                bridge_csr_base[5 + (i*8)]= 0x0;
        }
//      printk(KERN_ALERT "CSR_BDRs_configured with a 64GB window.\n");

}



void bridge_read_BDF_entry(struct unimem_info * info, int BDF_entry){ //and return the previous index so that the TLB can be updated
        int i;
        volatile u32 __iomem * bridge_csr_base; //a temp buffer variable to read/write
        bridge_csr_base = info->port.start + BRIDGE_CSR_BDF0_BASE;
        printk(KERN_ALERT "Reading the CSR in address.. %lx\n", bridge_csr_base);
        for(i=0;i<8;i++){

        printk(KERN_ALERT "CSR_BDR%d_REG0: 0x%x\n",i, bridge_csr_base[0 + i*8]);
        printk(KERN_ALERT "CSR_BDR%d_REG1: 0x%x\n",i, bridge_csr_base[1 + i*8]);
        printk(KERN_ALERT "CSR_BDR%d_REG2: 0x%x\n",i, bridge_csr_base[2 + i*8]);
        printk(KERN_ALERT "CSR_BDR%d_REG3: 0x%x\n",i, bridge_csr_base[3 + i*8]);
        printk(KERN_ALERT "CSR_BDR%d_REG4: 0x%x\n",i, bridge_csr_base[4 + i*8]);
        printk(KERN_ALERT "CSR_BDR%d_REG5: 0x%x\n",i, bridge_csr_base[5 + i*8]);
        }

}





int unimemTo_find_eviction_candidate(struct unimem_info * info,unsigned int TLB){ //and return the previous index so that the TLB can be updated

        volatile int candidate;
        volatile u32 __iomem *to_apb_base; //a temp buffer variable to read/write
//      printk(KERN_ALERT "unimem_find_eviction_candidate\n");
        to_apb_base = info->port.start + UNIMEM_TO_APB_OFFSET;
        //printk( "APB addr is %lx\n",  to_apb_base);
        //      printk( "Eviction candidate b4 is %lx\n",       to_apb_base[3]);

        to_apb_base[2] = 0x2; // write to the operation register that the operation is "get replacement candidate"
        candidate = to_apb_base[3];
//      printk( "Eviction candidate is %lx\n",  candidate);
        return candidate;
}


int unimemTo_replace_entry(struct unimem_info * info, unsigned int TLB, unsigned int index, unsigned int data0, unsigned int data1, unsigned int data2, unsigned int data3, unsigned int data4){

        volatile u32 __iomem *to_apb_base;
//      printk(KERN_ALERT "unimem_replace_entry\n");
        to_apb_base = info->port.start + UNIMEM_TO_APB_OFFSET; //the base address of the APB interface
        //printk( "APB addr is %lx\n",  to_apb_base);

        //Write INDEX_REGISTER (See 3.3.4).
        to_apb_base[3] = index;
        printk(KERN_ALERT "unimemTo_replace_entry: Replacing index %d\n", index);

        //Write data to DATA[0-4] REGISTERS.
        to_apb_base[4] = data0;
        to_apb_base[5] = data1;
        to_apb_base[6] = data2;
        to_apb_base[7] = data3;
        to_apb_base[8] = data4;

        // write to the operation register that the operation is "update"
        to_apb_base[2] = 0x0;

return 0;
}

int unimemTo_replace(struct unimem_info * info, unsigned int TLB, unsigned int data0, unsigned int data1, unsigned int data2, unsigned int data3, unsigned int data4){

        volatile int candidate;
        volatile u32 __iomem *to_apb_base; //a temp buffer variable to read/write
//      printk(KERN_ALERT "unimem_replace\n");
        to_apb_base = info->port.start + UNIMEM_TO_APB_OFFSET;
        //printk( "APB addr is %lx\n",  to_apb_base);
        //      printk( "Eviction candidate b4 is %lx\n",       to_apb_base[3]);
                printk(KERN_ALERT "data1::  %x\n", data1);

        candidate = unimemTo_find_eviction_candidate(info,TLB);// return the previous index so that the TLB can be updated
        unimemTo_replace_entry(info, TLB, candidate, data0, data1, data2, data3, data4); //we pass on the index, and replace that entry.
        return candidate;
}


/*
*
*
Starting the uFrom signatures
*
*
*/



int unimemFrom_find_eviction_candidate(struct unimem_info * info,unsigned int TLB){ //and return the previous index so that the TLB can be updated

        volatile int candidate;
        volatile u32 __iomem *to_apb_base; //a temp buffer variable to read/write
//      printk(KERN_ALERT "unimem_find_eviction_candidate\n");
        to_apb_base = info->port.start + UNIMEM_FROM_APB_OFFSET;
        //printk( "APB addr is %lx\n",  to_apb_base);
        //      printk( "Eviction candidate b4 is %lx\n",       to_apb_base[3]);

        to_apb_base[2] = 0x2; // write to the operation register that the operation is "get replacement candidate"
        candidate = to_apb_base[3];
//      printk( "Eviction candidate is %lx\n",  candidate);
        return candidate;
}




void unimemFrom_enable(struct unimem_info * info){

        volatile u32 __iomem *to_apb_base; //a temp buffer variable to read/write
        printk(KERN_ALERT "unimem_From_enabled\n");
        to_apb_base = info->port.start + UNIMEM_FROM_APB_OFFSET;


        to_apb_base[0] = 0x0;
        return;
}


void unimemTo_enable(struct unimem_info * info){

        volatile u32 __iomem *to_apb_base; //a temp buffer variable to read/write
        printk(KERN_ALERT "unimem_To_enabled\n");
        to_apb_base = info->port.start + UNIMEM_TO_APB_OFFSET;


        to_apb_base[0] = 0x0;
        return;
}


int unimemFrom_replace_entry(struct unimem_info * info, unsigned int TLB, unsigned int index, unsigned int data0, unsigned int data1, unsigned int data2, unsigned int data3, unsigned int data4){

        volatile u32 __iomem *to_apb_base;
//      printk(KERN_ALERT "unimemFrom_replace_entry\n");
        to_apb_base = info->port.start + UNIMEM_FROM_APB_OFFSET; //the base address of the APB interface
        //printk( "APB addr is %lx\n",  to_apb_base);

        //Write INDEX_REGISTER (See 3.3.4).
        to_apb_base[3] = index;
        printk(KERN_ALERT "unimemFrom_replace_entry: Replacing index %d\n", index);

        //Write data to DATA[0-4] REGISTERS.
        to_apb_base[4] = data0;
        to_apb_base[5] = data1;
        to_apb_base[6] = data2;
        to_apb_base[7] = data3;
        to_apb_base[8] = data4;

        // write to the operation register that the operation is "update"
        to_apb_base[2] = 0x0;

return 0;
}

int unimemFrom_replace(struct unimem_info * info, unsigned int TLB, unsigned int data0, unsigned int data1, unsigned int data2, unsigned int data3, unsigned int data4){

        volatile int candidate;
        volatile u32 __iomem *to_apb_base; //a temp buffer variable to read/write
//      printk(KERN_ALERT "unimemFrom_replace\n");
        to_apb_base = info->port.start + UNIMEM_FROM_APB_OFFSET;
        //printk( "APB addr is %lx\n",  to_apb_base);
        //      printk( "Eviction candidate b4 is %lx\n",       to_apb_base[3]);
        candidate = unimemFrom_find_eviction_candidate(info,TLB);// return the previous index so that the TLB can be updated
        unimemFrom_replace_entry(info, TLB, candidate, data0, data1, data2, data3, data4); //we pass on the index, and replace that entry.
        return candidate;
}



int virt_to_uni(int MSB, int LSB){

        return 0;
}


void translate_and_setup_reg(struct unimem_info * info, int offset, unsigned long int data, int accel_no, unsigned long long int phys_address[], unsigned long int virt_address[], int remote_allocations){
//translate the address and set up the register.
        volatile u32 __iomem *accel_addr;
        int i, flag;
        unsigned int ms_bits, ls_bits;
        unsigned long int user_last_bits = (data & 0xFFFFFFFfC0000000) >> 28;
        unsigned long int va_last_bits;
        accel_addr = info->port.start + accelerators_base[accel_no] + offset;
                flag = 0;
        for(i=0;i < remote_allocations;i++){
                va_last_bits = (virt_address[i] & 0xFFFFFFFfC0000000) >> 28;
         //       printk("Data %lx vs Virt_addr: %lx \n",data, virt_address[i]) ;


                if(user_last_bits == va_last_bits){
                        printk("Translation found, translating address: %lx , from data: %lx\n",virt_address[i],data);
//                                              printk("Physical address to be written is: %lx", phys_address[i]);
                                                flag = 1;
                                                break;
                                }
        }
                if(flag == 1){
                        accel_addr = info->port.start + accelerators_base[accel_no] + offset;
        //      printk("accel_addr: %lx\n",accel_addr);
        //      printk("data LSB: %lx\n",(unsigned int)(data)); //LSB
        //      printk("data MSB: %x\n",(unsigned int)(((unsigned int)(data>>32)))); //MSB;

        //              printk("accel_addr[0] b4: %x\n",accel_addr[0]); //MSB;
        //      printk("accel_addr[1] b4: %x\n",accel_addr[1]); //MSB;
                        //printk("phys_addr is: %x", phys_address[i]);
                        ls_bits = (phys_address[i] & 0x000000003FFFFFFF) + (data & 0x000000003FFFFFFF);
                        ms_bits = (phys_address[i] & 0xFFFFFFFC00000000) >> 32;
                //      printk("ms_bits is: %x", ms_bits);
                //      printk("ls_bits is: %x", ls_bits);
                        accel_addr[0] = ls_bits;
                        accel_addr[1] = ms_bits;
                        printk("translated reg[0] after: %x\n",accel_addr[0]); //MSB;
                        printk("translated reg[1] after: %x\n",accel_addr[1]); //MSB;
                }

        return;
}

void setup_reg(struct unimem_info * info, int offset,long int data, int accel_no){
        //just set up the register.
        //write to the accelerator base address + offset.
        volatile u32 __iomem *accel_addr;
        accel_addr = info->port.start + accelerators_base[accel_no] + offset;
        //printk("accel_addr: %x\n",accel_addr);
        accel_addr[0]  = data;
                printk("reg after: %x\n",accel_addr[0]); //MSB;
        return;
}

int setup_accelerator(struct unimem_info * info, struct acc_data * accel_data, int acc_registers_vector[][24], int * acc_registers_vector_size, unsigned long long int phys_address[], unsigned long int virt_address[], int remote_allocations){
        int accel_no = accel_data->accel_number;
        int i,j, flag;
        if(accel_data->reg_number > 24){
                printk(KERN_ALERT "The number of registers is too big!\n");
                return -1;
        }
                printk(KERN_ALERT "No of registers provided by the user:: %d\n",accel_data->reg_number);
        for(i=0;i < accel_data->reg_number;i++){ //traverse the provided register array, check if any of these registers are address registers
//      and do the translation accordingly ;)
                                printk("i: %d, reg: %x, value: %x\n",i,accel_data->reg_offset[i], accel_data->reg_data[i]);
                                flag = 0;
                for(j=0;j<acc_registers_vector_size[accel_data->accel_number]*2;j++){

                        if(accel_data->reg_offset[i] ==  acc_registers_vector[accel_data->accel_number][j]) //check if each of the user data match any of the data for that accelerator
                        {               flag = 1;
                                                                printk("The user provided offset is: %x, translating...\n",accel_data->reg_offset[i]);
                                translate_and_setup_reg(info, accel_data->reg_offset[i], accel_data->reg_data[i], accel_no, phys_address, virt_address, remote_allocations);//we need the accelerator number, the register offset and the data(address)
                                                                break;
                                                }
                }
                                if(flag == 0){
                                setup_reg( info, accel_data->reg_offset[i], accel_data->reg_data[i], accel_no); // else just setup
                                }
        }
        return 0;
}


int poll_accel(struct unimem_info * info, int accel_no){

        volatile u32 __iomem *accel_addr;

        accel_addr = info->port.start + accelerators_base[accel_no];
        return accel_addr[0];

}

