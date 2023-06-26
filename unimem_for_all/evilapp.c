#include <stdlib.h>

#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <memory.h>

volatile int *hw_mmap = NULL; /*evil global variable method*/
const char memDevice[] = "/dev/mem";
int main() {
/* open /dev/mem and error checking */
int i;
int file_handle = open(memDevice, O_RDWR | O_SYNC);

if (file_handle < 0) {
    printf("Failed to open /dev/mem\n\r");
    return errno;
}

/* mmap() the opened /dev/mem */
hw_mmap = (int *) (mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, file_handle, 0x0000));
if(hw_mmap ==(void*) -1) {

    fprintf(stderr,"map_it: Cannot map memory into user space.\n");
    return errno;
}
return 0;
}
