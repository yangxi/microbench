#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static void bind_processor(int cpu)
{
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
}

void nt_spin(void)
{
  int a;
  int b;
  while(1){
    __asm__ __volatile__("movnti %%eax, %0\n\t"
			 "movnti %%eax, %1\n\t"
			 "mfence\n\t":"+m"(a),"+m"(b));
  }
}

int
main(int argc, char **argv)
{
  int cpuid = atoi(argv[1]);
  printf("bind to cpu %d\n", cpuid);
  bind_processor(cpuid);
  nt_spin();
}
