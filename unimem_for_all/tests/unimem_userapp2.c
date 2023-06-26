#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define PAGE_SIZE 4096
#define GIGABYTE 0x40000000



#define UNIMEM_GVAS_BASE 0x10000000000

typedef struct {
    int region;
    int length;
    int node;
    int context;
    long long int base_addr;
    } unimem_data;

#define PASS_STRUCT _IOW('q', 'a', unimem_data *)

int main(int argc, char** argv)
{
    int i;
    int configfd;
    unsigned int nodeNo;
    nodeNo = 5;
    unimem_data unimem_ops;

    unimem_ops.node = nodeNo;
    unimem_ops.length = GIGABYTE ; //1GB
    unimem_ops.region = 0x1; //0 for host dram, 1 for fpga resources (BRAM-DDR), 2 for accelerators. Each one occupies a unimem-from index
    unimem_ops.context = 0x0;
    unimem_ops.base_addr = UNIMEM_GVAS_BASE + (GIGABYTE*(long long int)(unimem_ops.node + unimem_ops.region)); //base unimem GVAS

    configfd = open("/dev/unimem_file-5", O_RDWR); //because we are node 0.(for now)

    if (configfd < 0) {
        perror("open error");
        return -1;
    }




    if (ioctl(configfd, PASS_STRUCT, (unimem_data *)&unimem_ops) == -1)

        perror("query_apps ioctl set");

    unsigned int* buffer = NULL;
    buffer = mmap(unimem_ops.base_addr, GIGABYTE, PROT_READ | PROT_WRITE, MAP_SHARED /*| MAP_HUGETLB   /*MAP_ANONYMOUS  | MAP_HUGETLB  /*| MAP_POPULATE*/ , configfd, // we need to be able to specify the node that we want to mmap to.
                   0);

    if (buffer == MAP_FAILED) {
        perror("mmap");
            printf("Damn\n\r");

        return -1;
    }
    printf("Buffer is: %p\n\r", buffer);
    buffer[0] = 0xbebe;
    buffer[1] = 0xbebe;
    printf("Buffer data0: %x\n\r", buffer[0]);
    printf("Buffer data1: %x\n\r", buffer[1]);

  getchar();
    for(i=0; i< (1024*1024*256); i++){
        buffer[i] = (0xbebe);
        //printf("i is :%d, Buffer location is: %x\n\r", i, &buffer[i*1024]);
      //  printf("Buffer data is: %x\n\r", buffer[i*1024]);
    }
    // buffer[2] = 0xdead2;
    // buffer[3] = 0xdead3;
    // buffer[4] = 0xdead4;
    // buffer[5] = 0xdead5;
    // buffer[6] = 0xdead6;

    // printf("\nbuffer[0] value is: %x\n\r",buffer[0]);

    // printf("\nbuffer[1] value is: %x\n\r",buffer[1]);

    // printf("\nbuffer[2] value is: %x\n\r",buffer[2]);

    // printf("\nbuffer[3] value is: %x\n\r",buffer[3]);

    // printf("\nbuffer[4] value is: %x\n\r",buffer[4]);

    // printf("\nbuffer[5] value is: %x\n\r",buffer[5]);

    // printf("\nbuffer[6] value is: %x\n\r",buffer[6]);



    // printf("initial message: %x\n", address);
    // memcpy(address + 1 , "0xcafe", 4);
    // printf("changed message: %x\n", address);

    close(configfd);
    return 0;
}

