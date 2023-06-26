#include <linux/types.h>
#include "unimem_defs.h"

#ifndef UNIMEMOPS_H
#define UNIMEMOPS_H

//finds the current node number. Used for the fopen.
int find_current_node(int address);

//finds the ip coordinates to fill the unimem coordinates field.
int find_ip_coordinates(int address);

//returns the coordination bits
int get_ip_coordinates(int index);

//returns the possible eviction candidate, 0xFFF on error
int unimemTo_find_eviction_candidate(struct unimem_info * info, unsigned int TLB); //and return the previous index so that the TLB can be updated


//Replaces a specific entry. Returns 0 on success, 0xFFFF on error
int unimemTo_replace_entry(struct unimem_info * info, unsigned int TLB, unsigned int index, unsigned int data0, unsigned int data1, unsigned int data2, unsigned int data3, unsigned int data4);


//Replaces and invalidates the next available candidate. Returns the replaced index, or 0xFFFF on error
int unimemTo_replace(struct unimem_info * info, unsigned int TLB, unsigned int data0, unsigned int data1, unsigned int data2, unsigned int data3, unsigned int data4);

//Invalidates a specific entry. Returns 0 on success, 0xFFFF on error.
int unimemTo_invalidate_entry(struct unimem_info * info, unsigned int TLB, unsigned int index);

void unimemTo_enable(struct unimem_info * info);

void unimemFrom_enable(struct unimem_info * info);



/*
*
*
Starting the uFrom signatures
*
*
*/

//returns the possible eviction candidate, 0xFFF on error
int unimemFrom_find_eviction_candidate(struct unimem_info * info, unsigned int TLB); //and return the previous index so that the TLB can be updated


//Replaces a specific entry. Returns 0 on success, 0xFFFF on error
int unimemFrom_replace_entry(struct unimem_info * info, unsigned int TLB, unsigned int index, unsigned int data0, unsigned int data1, unsigned int data2, unsigned int data3, unsigned int data4);


//Replaces and invalidates the next available candidate. Returns the replaced index, or 0xFFFF on error
int unimemFrom_replace(struct unimem_info * info, unsigned int TLB, unsigned int data0, unsigned int data1, unsigned int data2, unsigned int data3, unsigned int data4);

//Invalidates a specific entry. Returns 0 on success, 0xFFFF on error.
int unimemFrom_invalidate_entry(struct unimem_info * info, unsigned int TLB, unsigned int index);


/*
*
*
Starting the slave bridge signatures
*
*
*/


//Ideally, this function should one of the bridge entries to point to the base physical address returned by dma_alloc_coherent().
//For testing, only 1 region will be used
int bridge_set_BDF_entry(struct unimem_info * info, int BDF_entry, unsigned int phys_addr);


void bridge_read_BDF_entry(struct unimem_info * info, int BDF_entry);

int setup_accelerator(struct unimem_info * info, struct acc_data * accel_data, int acc_registers_vector[][24], int * acc_registers_vector_size, unsigned long long int phys_address[], unsigned long int virt_address[], int remote_allocations);

void setup_reg(struct unimem_info * info, int offset, long int data, int accel_no);

void translate_and_setup_reg(struct unimem_info * info, int offset, unsigned long int data, int accel_no, unsigned long long int phys_address[], unsigned long int virt_address[], int remote_allocations);

int virt_to_uni(int MSB, int LSB);

int poll_accel(struct unimem_info * info, int accel_no);

#endif

