#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#define	__USE_GNU
#include <sched.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/time.h>
#define _GNU_SOURCE
#include <sched.h>


volatile int produerflag = 0;
uint64_t begin =0;
uint64_t end =0;
uint64_t d1[8];
volatile int consumerflag = 0;



static __inline uint64_t
rdtsc(void)
{
	u_int32_t low, high;

	__asm __volatile("rdtsc" : "=a" (low), "=d" (high));
	return (low | ((u_int64_t)high << 32));
}

void producer()
{
  begin = rdtsc();
  asm volatile("pusha\n\t");
   
}

void consumer()
{
}


void test()
{
    uint64_t begin = rdtsc();
    asm volatile("pusha\n\t"
		 "popa\n\t");
    uint64_t end = rdtsc();
    printf("pusha popa takes %lld cycles\n", end - begin);			   
}

void pinCPU(int cpuid)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpuid, &cpuset);
    pthread_setaffinity_np(pthread_self(),sizeof(cpuset), &cpuset);
}




int main(int argc, char **argv)
{
  int i =0;
  pincCPU(1);

  /* printf("hello world\n"); */
  /* for(i=0;i<10;i++) */
  /*   test(); */
  /* return 0; */

  /* while(1){ */
  /* pinCPU(1);  */
  /* for(i=2;i<8;i=i+1){ */
  /*   uint64_t begin = rdtsc(); */
  /*   pinCPU(1); */
  /*   uint64_t end = rdtsc(); */
  /*   printf("moving to cpu %d, takes %lld cycles\n", i, end - begin);			    */
  /* } */
  /* } */
}
