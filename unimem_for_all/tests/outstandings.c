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
        //      printf("doing OK, size is: %d\n\r", size);
                _mm256_stream_si256 ((__m256i*)dst, _mm256_stream_load_si256((__m256i const*)src));
        //                      printf("went OK\n\r");i

//		__m256i dst2 = _mm256_stream_load_si256 ((__m256i*)src + 0);
}


void copy_data_512(char* dst, const char* src, size_t size)
{
        assert(size % 64 == 0);
        while(size) {
		_mm256_stream_si256((__m256i*)dst, _mm256_stream_load_si256((__m256i const*)src));
                _mm256_stream_si256((__m256i*)(dst+32), _mm256_stream_load_si256((__m256i const*)(src+32)));

		_mm_sfence();

//              __m256i dst2 = _mm256_stream_load_si256 ((__m256i*)src + 0);
                src += 64;
                dst += 64;
                size -= 64;
        }
}


void copy_data_1024(char* dst, const char* src, size_t size)
{
        assert(size % 128 == 0);
        while(size) {
                _mm256_stream_si256((__m256i*)dst, _mm256_stream_load_si256((__m256i const*)src));
                _mm256_stream_si256((__m256i*)(dst+32), _mm256_stream_load_si256((__m256i const*)(src+32)));
		_mm256_stream_si256((__m256i*)dst+64, _mm256_stream_load_si256((__m256i const*)src+64));
                _mm256_stream_si256((__m256i*)(dst+96), _mm256_stream_load_si256((__m256i const*)(src+96)));

                _mm_sfence();

//              __m256i dst2 = _mm256_stream_load_si256 ((__m256i*)src + 0);
                src += 128;
                dst += 128;
                size -= 128;
        }
}

void copy_data_2048(char* dst, const char* src, size_t size)
{
	assert(size % 128 == 0);
        while(size) {
                _mm256_stream_si256((__m256i*)dst, _mm256_stream_load_si256((__m256i const*)src));
                _mm256_stream_si256((__m256i*)(dst+32), _mm256_stream_load_si256((__m256i const*)(src+32)));
                _mm256_stream_si256((__m256i*)dst+64, _mm256_stream_load_si256((__m256i const*)src+64));
                _mm256_stream_si256((__m256i*)(dst+96), _mm256_stream_load_si256((__m256i const*)(src+96)));
                _mm256_stream_si256((__m256i*)dst+128, _mm256_stream_load_si256((__m256i const*)src+128));
                _mm256_stream_si256((__m256i*)(dst+160), _mm256_stream_load_si256((__m256i const*)(src+160)));
                _mm256_stream_si256((__m256i*)dst+192, _mm256_stream_load_si256((__m256i const*)src+192));
                _mm256_stream_si256((__m256i*)(dst+224), _mm256_stream_load_si256((__m256i const*)(src+224)));

                _mm_sfence();

//              __m256i dst2 = _mm256_stream_load_si256 ((__m256i*)src + 0);
                src += 256;
                dst += 256;
                size -= 256;
        }
}


void copy_data_128(char* dst, const char* src, size_t size)
{
        assert(size % 16 == 0);
        while(size) {
        //      printf("doing OK, size is: %d\n\r", size);
                _mm_stream_si128 ((__m128i*)dst, _mm_stream_load_si128((__m128i const*)src));
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
    volatile int * temparray;
    temparray = (int*)aligned_alloc(64, size * 1024*1024);
    if(mode == 1)
        printf("mode is %d, WRITE\n", mode);
    else if(mode == 0)
        printf("mode is %d, READ\n", mode);
    if(region == 0)
        printf("region %d, HOST DDR\n",region);
    else if(region == 1)
        printf("region is %d, FPGA DRAM\n",region);
    printf("Transfer size in bytes: %d\n", size);


    nodeNo = 5; //set 5 for local
    unimem_data unimem_ops;
    size2 = size;
    unimem_ops.node = nodeNo;
    unimem_ops.length = GIGABYTE ; //1GB
    unimem_ops.region = region; //0 for host dram, 1 for fpga resources (BRAM-DDR), 2 for accelerators. Each one occupies a unimem-from index
    unimem_ops.context = 0x0;
    unimem_ops.base_addr = UNIMEM_GVAS_BASE + ((GIGABYTE*(long long int)(unimem_ops.node) + (GIGABYTE*(long long int)unimem_ops.region))); //base unimem GVAS

    configfd = open("/dev/unimem_file", O_RDWR); //because we are node 0.(for now)

    if (configfd < 0) {
        perror("open error");
        return -1;
    }




    if (ioctl(configfd, PASS_STRUCT, (unimem_data *)&unimem_ops) == -1)

        perror("query_apps ioctl set");

    volatile int* buffer = NULL;
   // clock_gettime(CLOCK_MONOTONIC, &start);


    buffer = mmap(unimem_ops.base_addr, GIGABYTE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NONBLOCK | MAP_FIXED /*| MAP_HUGETLB   /*MAP_ANONYMOUS  | MAP_HUGETLB  /*| MAP_POPULATE*/ , configfd, // we need to be able to specify the node that we want to mmap to.
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



getchar();
    if(mode == 1){ // that means "WRITE"

        for(i=0;i< size; i++){
            temparray[i*64] = i*4; //initialize the array to be written to the remote destination
         //   printf("%d\n\r",i);
        }
    //    printf("First buffer[0] value before write is %d..\n",buffer[0]);
    //    printf("Last buffer[%d] value before write is %d..\n", (size/4)-1,buffer[(size/4)-1]);
        clock_gettime(CLOCK_MONOTONIC, &start);
        // for(i=0; i< (size/4); i++){
        //     buffer[i] = (i*4);
        // }
        memcpy(buffer,temparray,size);
        clock_gettime(CLOCK_MONOTONIC, &end);
  //      printf("First buffer[0] value after write is %d..\n",buffer[0]);
  //      printf("Last buffer[%d] value after write is %d..\n", (size/4)-1,buffer[(size/4)-1]);
    }
    else if(mode == 0){ // that means "READ"
        clock_gettime(CLOCK_MONOTONIC, &start);
        for(i=0;i<size;i++){
		copy_data_256(temparray + i*8, buffer + i*8,32);
		_mm_mfence();
	}
	 	 clock_gettime(CLOCK_MONOTONIC, &end);



	for(i=0;i<128;i++){
	        printf("Src[%d]: %d  Dst[%d]: %d\n",i*4,buffer[i], i*4, temparray[i]);

	}
        printf("First buffer[0] value read is %d..\n",temparray[0]);

        printf("Last buffer[%d] value read is %d..\n", (size/4)-1,temparray[(size/4)-1]);
    }


//        for(i=0;i<size/4;i++)
  //              printf("buffer[%d]: %x\n\r",i,buffer[i]);


    // printf("initial message: %x\n", address);
    // memcpy(address + 1 , "0xcafe", 4);
    // printf("changed message: %x\n", address);
    timeElapsed = timespecDiff(&end, &start);
    printf("Time elapsed in ns is: %ld\n", (timeElapsed/size));

//    timeElapsed = timespecDiff(&end2, &end);
//    printf("Time elapsed in ns is: %f\n", (double)timeElapsed);
//    timeElapsed = timespecDiff(&end3, &end2);
//    printf("Time elapsed in ns is: %f\n", (double)timeElapsed)
//    timeElapsed = timespecDiff(&end4, &end3);
//    printf("Time elapsed in ns is: %f\n", (double)timeElapsed);



    printf("Bandwidth (MB/s) achieved is: %lf\n", (double)(  ((size2 / 1000000) / (double)( (double)timeElapsed / 1000000000))*size));

    close(configfd);
    return 0;
}


