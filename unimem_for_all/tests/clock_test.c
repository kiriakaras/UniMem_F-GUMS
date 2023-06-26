#include <stdio.h>
#include <stdlib.h>




u_int64_t rdtsc()
{
   u_int32_t hi, lo;
   __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
   return ( (u_int64_t)lo)|( ((u_int64_t)hi)<<32 );
}

int main(){
    unsigned long long b4, after;
        b4 =rdtsc();
//        printf("hello\n\r");
        after = rdtsc();
        printf("Cycle difference is %ull\n\r", after-b4);
        printf("Time difference is %f\n\r", (double)(after-b4));



}

