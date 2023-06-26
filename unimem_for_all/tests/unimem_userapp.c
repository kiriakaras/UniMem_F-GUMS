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
                _mm256_stream_si256 ((__m256i*)dst, _mm256_stream_load_si256((__m256i const*)src));
        //                      printf("went OK\n\r");i

//              __m256i dst2 = _mm256_stream_load_si256 ((__m256i*)src + 0);
                src += 32;
                dst += 32;
                size -= 32;
        }
}


void copy_data_512(char* dst, const char* src, size_t size)
{
        assert(size % 64 == 0);
        while(size) {
//              _mm256_stream_si256((__m256i*)dst, _mm256_stream_load_si256((__m256i const*)src));
  //              _mm256_stream_si256((__m256i*)(dst+32), _mm256_stream_load_si256((__m256i const*)(src+32)));
                __m256i a = _mm256_stream_load_si256((__m256i const*)src + 0);
                __m256i b = _mm256_stream_load_si256((__m256i const*)src + 1);
                _mm_lfence();

                _mm256_stream_si256((__m256i*)dst, a);
                _mm256_stream_si256((__m256i*)dst + 1, b);
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

                __m256i a = _mm256_stream_load_si256((__m256i const*)src + 0);
                __m256i b = _mm256_stream_load_si256((__m256i const*)src + 1);
                __m256i c = _mm256_stream_load_si256((__m256i const*)src + 2);
                __m256i d = _mm256_stream_load_si256((__m256i const*)src + 3);
                _mm_lfence();

                _mm256_stream_si256((__m256i*)dst, a);
                _mm256_stream_si256((__m256i*)dst + 1, b);
                _mm256_stream_si256((__m256i*)dst + 2, c);
                _mm256_stream_si256((__m256i*)dst + 3, d);

                _mm_sfence();



//                _mm_mfence();
//              _mm256_stream_si256((__m256i*)dst, _mm256_stream_load_si256((__m256i const*)src));
  //              _mm256_stream_si256((__m256i*)(dst+32), _mm256_stream_load_si256((__m256i const*)(src+32)));
//              _mm256_stream_si256((__m256i*)dst+64, _mm256_stream_load_si256((__m256i const*)src+64));
  //              _mm256_stream_si256((__m256i*)(dst+96), _mm256_stream_load_si256((__m256i const*)(src+96)));

                //_mm_mfence();

//              __m256i dst2 = _mm256_stream_load_si256 ((__m256i*)src + 0);
                src += 128;
                dst += 128;
                size -= 128;
        }
}

void copy_data_2048(char* dst, char* src, size_t size)
{
        assert(size % 256 == 0);
        while(size) {

                __m256i a = _mm256_stream_load_si256((__m256i const*)src + 0);
                __m256i b = _mm256_stream_load_si256((__m256i const*)src + 1);
                __m256i c = _mm256_stream_load_si256((__m256i const*)src + 2);
                __m256i d = _mm256_stream_load_si256((__m256i const*)src + 3);
                __m256i e = _mm256_stream_load_si256((__m256i const*)src + 4);
                __m256i f = _mm256_stream_load_si256((__m256i const*)src + 5);
                __m256i g = _mm256_stream_load_si256((__m256i const*)src + 6);
                __m256i h = _mm256_stream_load_si256((__m256i const*)src + 7);
                _mm_lfence();



                _mm256_stream_si256((__m256i*)dst, a);
                _mm256_stream_si256((__m256i*)dst + 1, b);
                _mm256_stream_si256((__m256i*)dst + 2, c);
                _mm256_stream_si256((__m256i*)dst + 3, d);
                _mm256_stream_si256((__m256i*)dst + 4, e);
                _mm256_stream_si256((__m256i*)dst + 5, f);
                _mm256_stream_si256((__m256i*)dst + 6, g);
                _mm256_stream_si256((__m256i*)dst + 7, h);

                _mm_sfence();
                src += 256;
                dst += 256;
                size -= 256;
        }
}


void copy_data_4096(char* dst, const char* src, size_t size)
{
        assert(size % 512 == 0);
        while(size) {

                __m256i a = _mm256_stream_load_si256((__m256i const*)src + 0);
                __m256i b = _mm256_stream_load_si256((__m256i const*)src + 1);
                __m256i c = _mm256_stream_load_si256((__m256i const*)src + 2);
                __m256i d = _mm256_stream_load_si256((__m256i const*)src + 3);
                __m256i e = _mm256_stream_load_si256((__m256i const*)src + 4);
                __m256i f = _mm256_stream_load_si256((__m256i const*)src + 5);
                __m256i g = _mm256_stream_load_si256((__m256i const*)src + 6);
                __m256i h = _mm256_stream_load_si256((__m256i const*)src + 7);
                __m256i i = _mm256_stream_load_si256((__m256i const*)src + 8);
                __m256i j = _mm256_stream_load_si256((__m256i const*)src + 9);
                __m256i k = _mm256_stream_load_si256((__m256i const*)src + 10);
                __m256i l = _mm256_stream_load_si256((__m256i const*)src + 11);
                __m256i m = _mm256_stream_load_si256((__m256i const*)src + 12);
                __m256i n = _mm256_stream_load_si256((__m256i const*)src + 13);
                __m256i o = _mm256_stream_load_si256((__m256i const*)src + 14);
                __m256i p = _mm256_stream_load_si256((__m256i const*)src + 15);

                _mm_lfence();



                _mm256_stream_si256((__m256i*)dst, a);
                _mm256_stream_si256((__m256i*)dst + 1, b);
                _mm256_stream_si256((__m256i*)dst + 2, c);
                _mm256_stream_si256((__m256i*)dst + 3, d);
                _mm256_stream_si256((__m256i*)dst + 4, e);
                _mm256_stream_si256((__m256i*)dst + 5, f);
                _mm256_stream_si256((__m256i*)dst + 6, g);
                _mm256_stream_si256((__m256i*)dst + 7, h);
                _mm256_stream_si256((__m256i*)dst + 8, i);
                _mm256_stream_si256((__m256i*)dst + 9, j);
                _mm256_stream_si256((__m256i*)dst + 10, k);
                _mm256_stream_si256((__m256i*)dst + 11, l);
                _mm256_stream_si256((__m256i*)dst + 12, m);
                _mm256_stream_si256((__m256i*)dst + 13, n);
                _mm256_stream_si256((__m256i*)dst + 14, o);
                _mm256_stream_si256((__m256i*)dst + 15, p);

                _mm_sfence();

                src += 512;
                dst += 512;
                size -= 512;
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


    nodeNo = 4; //set 0 for local
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


    buffer = mmap(unimem_ops.base_addr, GIGABYTE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NONBLOCK | MAP_FIXED /* | MAP_HUGETLB   /*MAP_ANONYMOUS  | MAP_HUGETLB  /*| MAP_POPULATE*/ , configfd, // we need to be able to specify the node that we want to mmap to.
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

	//    temparray[0] = 0xcafe;
	//    temparray[1] = 0xcafe;
        for(i=0;i< size/4; i++){
            temparray[i] = i*4; //initialize the array to be written to the remote destination
         //   printf("%d\n\r",i);
        }
    //    printf("First buffer[0] value before write is %d..\n",buffer[0]);
    //    printf("Last buffer[%d] value before write is %d..\n", (size/4)-1,buffer[(size/4)-1]);
        clock_gettime(CLOCK_MONOTONIC, &start);
        // for(i=0; i< (size/4); i++){
        //     buffer[i] = (i*4);
        // }
    //    memcpy(buffer,temparray,size);
    //
    //
    ///

    /*    if(size == 8){
            for(i=0;i<1;i++){
            copy_data_64(buffer +262144, temparray, size);}
        }
        else if (size == 16)
                for (i=0;i<1;i++)
                copy_data_128(buffer +262144, temparray,size);

    //    else if(size % 512 == 0)
      //          for (i=0;i<1;i++)
        //            copy_data_4096(buffer, temparray,size);

        else if(size % 256 == 0)
                for (i=0;i<1;i++)
                    copy_data_2048(buffer +262144, temparray,size);

        else if(size % 128 == 0)
                for (i=0;i<1;i++)
                    copy_data_1024(buffer +262144, temparray,size);

        else if(size % 64 == 0)
                for (i=0;i<1;i++)
                    copy_data_512(buffer +262144, temparray,size);

        else if(size % 32 == 0)
                for (i=0;i<1;i++)
                    copy_data_256(buffer +262144, temparray,size);

        else
        for(i=0;i<1;i++)*/
                memcpy(buffer,temparray,size);
//buffer[262144] = 0xdeadbeef;
        clock_gettime(CLOCK_MONOTONIC, &end);
  //      printf("First buffer[0] value after write is %d..\n",buffer[0]);
  //      printf("Last buffer[%d] value after write is %d..\n", (size/4)-1,buffer[(size/4)-1]);
    }
    else if(mode == 0){ // that means "READ"
        clock_gettime(CLOCK_MONOTONIC, &start);
/*        if(size == 8){
            for(i=0;i<1;i++){
            copy_data_64(temparray, buffer +262144, size);}
        }
        else if (size == 16)
            for (i=0;i<1;i++)
                copy_data_128(temparray, buffer+ 262144,size);

        //    else if(size % 512 == 0)
          //      for (i=0;i<1;i++)
            //    copy_data_4096(buffer, temparray,size);

        else if(size % 256 == 0)
                for (i=0;i<1;i++)
                    copy_data_2048(temparray, buffer +262144,size);

        else if(size % 128 == 0)
                for (i=0;i<1;i++)
                    copy_data_1024(temparray, buffer+262144,size);

        else if(size % 64 == 0)
                for (i=0;i<1;i++)
                    copy_data_512(temparray, buffer +262144,size);

        else if(size % 32 == 0)
                for (i=0;i<1;i++)
                    copy_data_256(temparray, buffer +262144,size);

        */
        for(i=0;i<1;i++){
                memcpy(temparray,buffer,size);
        }


//_mm256_store_si256((__m256i*)temparray, _mm256_load_si256((__m256i const*)buffer));
                //_mm256_stream_si256 ((__m256i*)temparray, _mm256_stream_load_si256((__m256i const*)buffer));
                //_mm256_stream_si256 ((__m256i*)temparray+8, _mm256_stream_load_si256((__m256i const*)buffer+8));

                //_mm256_stream_si256 ((__m256i*)temparray+16, _mm256_stream_load_si256((__m256i const*)buffer+16));
                // _mm256_stream_si256 ((__m256i*)temparray+24, _mm256_stream_load_si256((__m256i const*)buffer+24));
 //_mm_mfence();

                 clock_gettime(CLOCK_MONOTONIC, &end);
  //      clock_gettime(CLOCK_MONOTONIC, &end2);
  //      clock_gettime(CLOCK_MONOTONIC, &end3);
//      clock_gettime(CLOCK_MONOTONIC, &end4);

//                _mm256_store_si256((__m256i*)temparray, _mm256_load_si256((__m256i const*)buffer));
    //            _mm256_store_si256((__m256i*)temparray+32, _mm256_load_si256((__m256i const*)buffer+32));

 //               _mm256_store_si256((__m256i*)temparray+64, _mm256_load_si256((__m256i const*)buffer+64));
   //             _mm256_store_si256((__m256i*)temparray+96, _mm256_load_si256((__m256i const*)buffer+96));

  //              _mm_mfence();

    }
//        printf("First buffer[0] value read is %d..\n",temparray[0]);

  //      printf("Last buffer[%d] value read is %d..\n", (size/4)-1,temparray[(size/4)-1]);


//        for(i=0;i<size/4;i++)
  //              printf("buffer[%d]: %x\n\r",i,buffer[i]);


    // printf("initial message: %x\n", address);
    // memcpy(address + 1 , "0xcafe", 4);
    // printf("changed message: %x\n", address);
    timeElapsed = timespecDiff(&end, &start);
    printf("Time elapsed in ns is: %ld\n", (timeElapsed/1));

//    timeElapsed = timespecDiff(&end2, &end);
//    printf("Time elapsed in ns is: %f\n", (double)timeElapsed);
//    timeElapsed = timespecDiff(&end3, &end2);
//    printf("Time elapsed in ns is: %f\n", (double)timeElapsed)
//    timeElapsed = timespecDiff(&end4, &end3);
//    printf("Time elapsed in ns is: %f\n", (double)timeElapsed);



    printf("Bandwidth (MB/s) achieved is: %lf\n", (double)(  ((size2 / 1000000) / (double)( (double)timeElapsed / 1000000000))*1));


    	getchar();

        for(i=0;i<size/4;i++)
                printf("Src[%d]: %d, Dst[%d]: %d\n", i*4, buffer[i], i*4,temparray[i]);

        printf("First buffer[0] value read is %x..\n",temparray[0]);

     //   printf("Last buffer[%d] value read is %d..\n", (size/4)-1,temparray[(size/4)-1]);

    //    for(i=0;i<size/4;i++)
      //          printf("Src[%d]: %d  Dst[%d]: %d\n",i*4,buffer[i], i*4, temparray[i]);


    close(configfd);
    return 0;
}



