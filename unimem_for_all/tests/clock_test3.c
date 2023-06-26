#include <stdio.h>
#include <stdlib.h>



static  __inline__ u_int64_t rdtsc(void)
   {

u_int32_t lo, hi;
__asm__ __volatile__ (
        "xorl %%eax,%%eax \n        cpuid"
        ::: "%rax", "%rbx", "%rcx", "%rdx");
__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
return (u_int64_t)hi << 32 | lo;
 }


int main(){
    unsigned long long b4, after;
        b4 =rdtsc();
//        printf("hello\n\r");
        after = rdtsc();
        printf("Cycle difference is %ull\n\r", after-b4);
        printf("Time difference is %f\n\r", (double)(after-b4));



}

