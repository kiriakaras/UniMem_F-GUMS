#include <x86intrin.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

#define PASS_STRUCT _IOW('q', 'd', unimem_data *)

int64_t timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p)
{
  return ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
}

void copy_data_256(char* dst, const char* src, size_t size)
{
        assert(size % 32 == 0);
        while(size) {
        //      printf("doing OK, size is: %d\n\r", size);
                _mm256_store_si256 ((__m256i*)dst, _mm256_load_si256((__m256i const*)src));
        //                      printf("went OK\n\r");

                src += 32;
                dst += 32;
                size -= 32;
        }
}


void copy_data_128(char* dst, const char* src, size_t size)
{
        assert(size % 16 == 0);
        while(size) {
        //      printf("doing OK, size is: %d\n\r", size);
                _mm_store_si128 ((__m128i*)dst, _mm_load_si128((__m128i const*)src));
        //                      printf("went OK\n\r");

                src += 16;
                dst += 16;
                size -= 16;
        }
}


void copy_data_64(char* dst, const char* src, size_t size)
{
        assert(size % 8 == 0);
        while(size) {
        //      printf("doing OK, size is: %d\n\r", size);
                _mm_storel_epi64 ((__m128i*)dst, _mm_loadl_epi64((__m128i const*)src));
        //                      printf("went OK\n\r");

                src += 8;
                dst += 8;
                size -= 8;
        }
}


void copy_data_32(char* dst, const char* src, size_t size)
{
        assert(size % 4 == 0);
        while(size) {
        //      printf("doing OK, size is: %d\n\r", size);
                _mm_store_ss ((__m128i*)dst, _mm_load_ss((__m128i const*)src));
        //                      printf("went OK\n\r");

                src += 4;
                dst += 4;
                size -= 4;
        }
}






int main(int argc, char** argv)
{
    struct timespec start, end, end2, end3, end4;
    int64_t timeElapsed;
    int i;
    int configfd;
    unsigned int nodeNo;
    int mode;
    int size;
    double size2;
    int region;
    volatile int temp = -1;
    int arg;
    if (argc > 1){
        for (arg = 1; arg < argc; ++arg)
        {
            if(arg == 1){
                mode = atoi(argv[1]);
            }
            else if(arg == 2){
                region = atoi(argv[2]);
            }
            else if(arg == 3){
                size = atoi(argv[3]);
            }
        }
    }
    int * temparray;
    temparray = (int*)aligned_alloc(64, size);
    if(mode == 1)
        printf("mode is %d, WRITE\n", mode);
    else if(mode == 0)
        printf("mode is %d, READ\n", mode);
    if(region == 0)
        printf("region %d, HOST DDR\n",region);
    else if(region == 1)
        printf("region is %d, FPGA DRAM\n",region);
    printf("Transfer size in bytes: %d\n", size);


    nodeNo = 1;
    unimem_data unimem_ops;
    size2 = size;
    unimem_ops.node = nodeNo;
    unimem_ops.length = GIGABYTE ; //1GB
    unimem_ops.region = region; //0 for host dram, 1 for fpga resources (BRAM-DDR), 2 for accelerators. Each one occupies a unimem-from index
    unimem_ops.context = 0x0;
    unimem_ops.base_addr = UNIMEM_GVAS_BASE + (GIGABYTE*(long long int)(unimem_ops.node + unimem_ops.region)); //base unimem GVAS

    configfd = open("/dev/unimem_file", O_RDWR); //because we are node 0.(for now)

    if (configfd < 0) {
        perror("open error");
        return -1;
    }




    if (ioctl(configfd, PASS_STRUCT, (unimem_data *)&unimem_ops) == -1)

        perror("query_apps ioctl set");

    volatile int* buffer = NULL;
   // clock_gettime(CLOCK_MONOTONIC, &start);


    buffer = mmap(unimem_ops.base_addr, GIGABYTE, PROT_READ | PROT_WRITE, MAP_SHARED /*| MAP_HUGETLB   /*MAP_ANONYMOUS  | MAP_HUGETLB  /*| MAP_POPULATE*/ , configfd, // we need to be able to specify the node that we want to mmap to.
                   0);

    if (buffer == MAP_FAILED) {
        perror("mmap");
            printf("Damn\n\r");

        return -1;
    }
            //clock_gettime(CLOCK_MONOTONIC, &end);

    //timeElapsed = timespecDiff(&end, &start);
    //printf("Time elapsed in ns is: %ld\n", (timeElapsed));

    //printf("Press to start the transfer...\n\r");

   // getchar();

    if(mode == 1){ // that means "WRITE"

        for(i=0;i< size/4; i++){
            temparray[i] = i*4; //initialize the array to be written to the remote destination
	 //   printf("%d\n\r",i);
        }
        printf("First buffer[0] value before write is %d..\n",buffer[0]);
        printf("Last buffer[%d] value before write is %d..\n", (size/4)-1,buffer[(size/4)-1]);
        clock_gettime(CLOCK_MONOTONIC, &start);
        // for(i=0; i< (size/4); i++){
        //     buffer[i] = (i*4);
        // }
        memcpy(buffer,temparray,size);
        clock_gettime(CLOCK_MONOTONIC, &end);
        printf("First buffer[0] value after write is %d..\n",buffer[0]);
        printf("Last buffer[%d] value after write is %d..\n", (size/4)-1,buffer[(size/4)-1]);
    }
    else if(mode == 0){ // that means "READ"
        // for(i=0; i< size/4); i++){
        // temp = buffer[i];
        // }
        //
        //
        clock_gettime(CLOCK_MONOTONIC, &start);
        if(size == 8){
            for(i=0;i<1000;i++){
            copy_data_64(temparray, buffer, size);}
        }
        else if (size == 16){
            for (i=0;i<1000;i++){
                copy_data_128(temparray, buffer,size);
            }
        }
        else if(size % 32 == 0){
                for (i=0;i<1000;i++){
                    copy_data_256(temparray, buffer,size);}}
        else
        for(i=0;i<1000;i++){
                memcpy(temparray,buffer,size);
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
  //      clock_gettime(CLOCK_MONOTONIC, &end2);
  //      clock_gettime(CLOCK_MONOTONIC, &end3);
//      clock_gettime(CLOCK_MONOTONIC, &end4);



        printf("First buffer[0] value read is %d..\n",buffer[0]);

        printf("Last buffer[%d] value read is %d..\n", (size/4)-1,temparray[(size/4)-1]);
    }


//        for(i=0;i<size/4;i++)
  //              printf("buffer[%d]: %x\n\r",i,buffer[i]);


    // printf("initial message: %x\n", address);
    // memcpy(address + 1 , "0xcafe", 4);
    // printf("changed message: %x\n", address);
    timeElapsed = timespecDiff(&end, &start);
    printf("Time elapsed in ns is: %ld\n", (timeElapsed/1000));

//    timeElapsed = timespecDiff(&end2, &end);
//    printf("Time elapsed in ns is: %f\n", (double)timeElapsed);
//    timeElapsed = timespecDiff(&end3, &end2);
//    printf("Time elapsed in ns is: %f\n", (double)timeElapsed)
//    timeElapsed = timespecDiff(&end4, &end3);
//    printf("Time elapsed in ns is: %f\n", (double)timeElapsed);



    printf("Bandwidth (MB/s) achieved is: %lf\n", (double)(  ((size2 / 1000000) / (double)( (double)timeElapsed / 1000000000))*1000));

    close(configfd);
    return 0;
}


