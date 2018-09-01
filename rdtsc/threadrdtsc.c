#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sched.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>

//usage threadrdtsc bufsize

#define NR_SAMPLES (100)

volatile unsigned long long timestamp = 0;

unsigned long long rdtsc(void)
{
  unsigned int tickl, tickh;
  __asm__ __volatile__("rdtsc":"=a"(tickl),"=d"(tickh)::"%ecx");
  return ((unsigned long long)tickh << 32)|tickl;
}

void * profiler(void * arg){

  while(1){
    timestamp++;
  }
  
  /*  asm volatile("movq $0, %%rax\n\t"
	       "movq $0, %%rbx\n\t"
	       "S:\n\t"
	       "incq %%rax\n\t"
	       "movl (%0, %%rbx, 4), %%ecx\n\t"
	       "testl %%ecx, %%ecx\n\t"
	       "je S\n\t"
	       "movq %%rax, (%1, %%rbx, 8)\n\t"
	       "incq %%rbx\n\t"
	       "jmp S\n\t"::"r"(stream), "r"(tag):"%rax","rbx");*/

}


double kernel(double *buf, int size){
  int i =0;
  double total = 0;
  for (i=0;i < size; i++){
    total += total + buf[i];
  }
  return total / size;
}



int
main(int argc, char **argv)
{
  pthread_t p;
  pthread_attr_t attr;
  unsigned long long harddiff[NR_SAMPLES];
  unsigned long long softdiff[NR_SAMPLES];

  for(int i=0; i<NR_SAMPLES; i++){
    harddiff[i] = 0;
    softdiff[i] = 0;
  }

  printf("create profiling thread\n");
  pthread_attr_init(&attr);
  pthread_create(&p, &attr, &profiler,NULL);
  sleep(1);

  int nr_change = 0;
  int nr_loop = 0;
  unsigned long long lastsoftstamp = timestamp;
  unsigned long long lasthardstamp = rdtsc();
  while(1){
    unsigned long long softstamp = timestamp;
    unsigned long long hardstamp = rdtsc();
    if (softstamp != lastsoftstamp){
      softdiff[nr_change]  = softstamp - lastsoftstamp;
      harddiff[nr_change] =  hardstamp - lasthardstamp;
      lastsoftstamp = softstamp;
      lasthardstamp = hardstamp;
      nr_change += 1;
      if (nr_change == 100)
	break;      
      nr_loop += 1;
    }
  }

  printf("reporting 100 changes in %d loops\n", nr_loop);
  for(int i=0;i<NR_SAMPLES; i++){
    double softratio = 0.0;
    double hardratio = 0.0;
    if (i > 0){
      softratio = (double)softdiff[i] / (double)softdiff[i-1];
      hardratio = (double)harddiff[i] / (double)harddiff[i-1];
    }
    
    printf("softdiff %lld ratio %f, harddiff %lld ratio %f\n", softdiff[i], softratio, harddiff[i], hardratio);
  }
}
