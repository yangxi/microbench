#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static unsigned long __inline__ rdtscp(void)
{
  unsigned int tickl, tickh;
  __asm__ __volatile__("rdtscp":"=a"(tickl),"=d"(tickh)::"%ecx");
  return ((uint64_t)tickh << 32)|tickl;
}

void nt_copy(int val, unsigned long *dst, int size)
{
  for (int i=0; i<size; i++)
    __builtin_ia32_movntq((long long unsigned int *)(dst + i), (long long unsigned int) val);
}

int
main(int argc, char **argv)
{
  volatile char *buf = (char *)malloc(10*1024*1024);
  int ret = 0;
  for(int t=0; t<10; t++) {
    nt_copy(0, (unsigned long *)buf, 10*1024*1024 / sizeof(long long unsigned int));
    long  pt0 = rdtscp();
    //    for(int i=0; i<10*1024*1024; i= i+4*1024)
    //            __builtin_prefetch((char *)buf + i);
    //buf[i] = i;
    long pt1 = rdtscp() - pt0;

    long t0 = rdtscp();
    for(int i=0; i<10*1024*1024; i= i+4*1024){
      ret += buf[i];
      __builtin_prefetch((char *)buf + i + 4*1024);
      //      buf[i+1024] = i;
      //      buf[i+2048] = i;
    }
    long t1 = rdtscp() - t0;
    printf("timestamp:%ld %ld\n", pt1,t1);
  }
  return ret;
}
