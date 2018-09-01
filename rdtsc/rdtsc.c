#include <stdio.h>
#include <stdlib.h>
#include <string.h>


unsigned long long __inline__ rdtsc (void)
{
  unsigned int tickl, tickh;
  __asm__ __volatile__("rdtscp":"=a"(tickl),"=d"(tickh)::"%ecx");
  return ((unsigned long long)tickh << 32)|tickl;
}


double kernel(double *buf, int size, unsigned long long * cycle)
{
  double total = 0.0;
  volatile unsigned long long begin = rdtsc();
  //  for (int j=0; j<1024; j++){
  //    total += buf[j];
  //  }
  /* __asm__ __volatile__("inc %%rcx\n\t" */
  /* 		       "inc %%ecx\n\t" */
  /* 		       "inc %%ecx\n\t" */
  /* 		       "inc %%ecx\n\t" */
  /* 		       "inc %%ecx\n\t" */
  /* 		       :::"%rcx"); */

  volatile unsigned long long end = rdtsc();
  *cycle = end - begin;
  return total/size;
}

int
main(int argc, char ** argv)
{
  double *buf = (double *) malloc( 1024*sizeof(double) );
  memset(buf, 3.14125, sizeof(1024 * sizeof(double)));

  double results[100];
  unsigned long long cycles[100];

  volatile unsigned long long begin = rdtsc();

  for (int i=0; i<50; i++){
    results[i] = kernel(buf, 1024, &(cycles[i]));
  }

  volatile unsigned long end = rdtsc();

  printf("total in %lld cycles\n", end - begin);

  for (int i=0; i<50; i++){
    printf("%lld\n", cycles[i]);
  }


}
