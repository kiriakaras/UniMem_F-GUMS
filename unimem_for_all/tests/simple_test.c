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
#define GIGABYTE 0x40000000UL



#define UNIMEM_GVAS_BASE 0x10000000000UL

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
	    else if(arg == 4){
	    	nodeNo = atoi(argv[4]);
	    }
        }
    }
    int * temparray;
    temparray = (int*)aligned_alloc(64, size);
	temparray = (int*)malloc(size);    
if(mode == 1)
        printf("mode is %d, WRITE\n", mode);
    else if(mode == 0)
        printf("mode is %d, READ\n", mode);
    if(region == 0)
        printf("region %d, HOST DDR\n",region);
    else if(region == 1)
        printf("region is %d, FPGA DRAM\n",region);
    printf("Transfer size in bytes: %d\n", size);

	if(nodeNo == 0)
        printf("Node selected is 21\n");
    else if(nodeNo == 1)
        printf("Node selected is 58\n");
    else if(nodeNo == 2)
        printf("Node selected is 59\n");
    else if(nodeNo == 3)
        printf("Node selected is 74\n");
     else if(nodeNo == 4)
         printf("Node selected is 80\n");


    //nodeNo = 0; //set 4 for local - 2 for 59
    unimem_data unimem_ops;
    size2 = size;
    unimem_ops.node = nodeNo;
    unimem_ops.length = GIGABYTE ; //1GB
    unimem_ops.region = region; //0 for host dram, 1 for fpga resources (BRAM-DDR), 2 for accelerators. Each one occupies a unimem-from index
    unimem_ops.context = 0x0;
    unimem_ops.base_addr = UNIMEM_GVAS_BASE + ((GIGABYTE*(long long int)(unimem_ops.node*3) + (GIGABYTE*(long long int)unimem_ops.region))); //base unimem GVAS




    configfd = open("/dev/unimem_file", O_RDWR); //because we are node 0.(for now)

    if (configfd < 0) {
        perror("open error");
        return -1;
    }




    if (ioctl(configfd, PASS_STRUCT, (unimem_data *)&unimem_ops) == -1)

        perror("query_apps ioctl set");

    volatile int* buffer = NULL;
   // clock_gettime(CLOCK_MONOTONIC, &start);


    buffer = mmap(unimem_ops.base_addr, GIGABYTE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NONBLOCK | MAP_FIXED /* | MAP_HUGETLB   /*MAP_ANONYMOUS  | MAP_HUGETLB  */| MAP_POPULATE , configfd, // we need to be able to specify the node that we want to mmap to.
                   0);

printf("unimem_ops.base_addr is: %lx", unimem_ops.base_addr);


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

        for(i=0;i< size/4; i++){
            temparray[i] = 0; //initialize the array to be written to the remote destination
            //printf("%d\n\r",i);
        }
    //    printf("First buffer[0] value before write is %d..\n",buffer[0]);
    //    printf("Last buffer[%d] value before write is %d..\n", (size/4)-1,buffer[(size/4)-1]);
        clock_gettime(CLOCK_MONOTONIC, &start);
//         for(i=0; i< (size/4); i++){
  //           buffer[i] = (i*4);
    //     }
    //    memcpy(buffer,temparray,size);
        for(i=0;i<1;i++)
                memcpy(buffer,temparray,size);
        clock_gettime(CLOCK_MONOTONIC, &end);
//        printf("First buffer[0] value after write is %d..\n",buffer[0]);
  //      printf("Last buffer[%d] value after write is %d..\n", (size/4)-1,buffer[(size/4)-1]);
    }
    else if(mode == 0){ // that means "READ"
          for(i=0;i< size/4; i++){
             temparray[i] = 0; //initialize the array to be written to the remote destination
      clock_gettime(CLOCK_MONOTONIC, &start);}
       
                 for (i=0;i<1;i++);
			memcpy(temparray,buffer,size);	 


                 clock_gettime(CLOCK_MONOTONIC, &end);

    }
//        printf("First buffer[0] value read is %d..\n",temparray[0]);

  //      printf("Last buffer[%d] value read is %d..\n", (size/4)-1,temparray[(size/4)-1]);


  //      for(i=0;i<size/4;i++)
    //            printf("buffer[%d]: %x\n\r",i,buffer[i]);


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


    //	getchar();

        for(i=0;i<size/4;i++)
                printf("Src[%d]: %x, Dst[%d]: %x\n", i, buffer[i], i,temparray[i]);

   //     printf("First buffer[0] value read is %x..\n",temparray[0]);

     //   printf("Last buffer[%d] value read is %d..\n", (size/4)-1,temparray[(size/4)-1]);

///        for(i=0;i<size/4;i++)
   ///             printf("Local buffer[%d] is: %d\n",i,temparray[i]);
//	for(i=0;i<size/4;i++)
  //      	printf("Remote buffer[%d] is: %d\n",i,buffer[i]);
	

    close(configfd);
    return 0;
}


