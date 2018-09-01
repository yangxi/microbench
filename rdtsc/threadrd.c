#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <perfmon/pfmlib_perf_event.h>

unsigned long flag = 0;
unsigned long pad[128];
int nr_loop = 0;
int nr_payload = 0;
unsigned long readcycles[128];
unsigned long  volatile nr_op=0;
unsigned long pad2[128];
unsigned long  volatile nr_source=0;
char *events[]={"L2_RQSTS:ALL_RFO"};
//char *events[]={"IDQ:ALL_MITE_UOPS"};
int nr_perf_events = 1;
static int perf_event_fds[10];
static struct perf_event_attr *perf_event_attrs;

void sysPerfEventInit(int numEvents)
{
  //  printf("sysPerfEventInit with %d events\n", numEvents);
  int ret = pfm_initialize();
  if (ret != PFM_SUCCESS) {
    errx(1, "error in pfm_initialize: %s", pfm_strerror(ret));
  }

  //  perf_event_fds = (int*)calloc(numEvents, sizeof(int));
  //  if (!perf_event_fds) {
  //    errx(1, "error allocating perf_event_fds");
  //  }
  perf_event_attrs = (struct perf_event_attr *)calloc(numEvents, sizeof(struct perf_event_attr));
  if (!perf_event_attrs) {
    errx(1, "error allocating perf_event_attrs");
  }
  int i;
  for(i=0; i < numEvents; i++) {
    perf_event_attrs[i].size = sizeof(struct perf_event_attr);
  }
}

void sysPerfEventCreate(int id, const char *eventName)
{
  struct perf_event_attr *pe = (perf_event_attrs + id);
  int ret = pfm_get_perf_event_encoding(eventName, PFM_PLM3, pe, NULL, NULL);
  if (ret != PFM_SUCCESS) {
    errx(1, "error creating event %d '%s': %s\n", id, eventName, pfm_strerror(ret));
  }
  pe->read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
  pe->disabled = 0;
  pe->inherit = 1;
  perf_event_fds[id] = perf_event_open(pe, 0, -1, -1, 0);
  if (perf_event_fds[id] == -1) {
    err(1, "error in perf_event_open for event %d '%s'", id, eventName);
  }
}

void sysPerfEventEnable()
{

    if (prctl(PR_TASK_PERF_EVENTS_ENABLE)) {
      err(1, "error in prctl(PR_TASK_PERF_EVENTS_ENABLE)");
    }

}

void sysPerfEventDisable()
{

    if (prctl(PR_TASK_PERF_EVENTS_DISABLE)) {
      err(1, "error in prctl(PR_TASK_PERF_EVENTS_DISABLE)");
    }

}

void sysPerfEventRead(int id, long long *values)
{
  size_t expectedBytes = 3 * sizeof(long long);
  int ret = read(perf_event_fds[id], values, expectedBytes);
  if (ret < 0) {
    err(1, "error reading event: %s", strerror(errno));
  }
  if (ret != expectedBytes) {
    errx(1, "read of perf event did not return 3 64-bit values, but return %d", ret);
  }
}



static unsigned long long __inline__ rdtsc(void)
{
  unsigned int tickl, tickh;
  __asm__ __volatile__("rdtsc":"=a"(tickl),"=d"(tickh)::"%ecx");
  return ((unsigned long long)tickh << 32)|tickl;
}

static void inline relax_cpu(){
  __asm__ volatile("rep; nop\n\t"::: "memory");
}


void gotocpu(int cpu)
{
  int s, j;
  cpu_set_t cpuset;
  pthread_t thread;
  thread = pthread_self();

  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

void payload(int cycles)
{
  unsigned long stamp = rdtsc();
  while(rdtsc() - stamp < cycles)
    ;
}

void  writeflag(unsigned int nrtimes)
{
  long ret = 0;
  int dummy = 0;
  int i,j;

  for(j=0;j<nrtimes;j++){
    for (i=0;i<nr_payload;i++){
      unsigned int tickl, tickh;
      __asm__ __volatile__("rdtsc":"=a"(tickl),"=d"(tickh)::"%ecx");
    }
    __asm __volatile__("movl $1, %1\n\t"
		       "movl $1, %0\n\t"
		       :"+m"(dummy),"+m"(flag)::"memory");
  }
}

void reader()
{
  //touch global flag for ever
  int i;
  int ret = 0xff;
  while(1){
    for(i=0;i<nr_loop;i++){
      unsigned int tickl, tickh;
      __asm__ __volatile__("rdtsc":"=a"(tickl),"=d"(tickh)::"%ecx");
    }

    asm volatile("mov %1,%0\n\t"
		 "incl %2\n\t"
		 :"=r"(ret):"m"(flag),"m"(nr_op):"memory");
  }
}

void readflag(unsigned int nrtimes)
{
  int dummy = 0;
  int read = 0;
  int i,j;
  for(j=0;j<nrtimes;j++){
    for (i=0;i<nr_payload;i++)
      __asm__ volatile("nop\n\t"::: "memory");

    asm volatile("movl %2, %0\n\t"
		 "movl $0, %1\n\t"
		 :"+r"(read):"m"(dummy),"m"(flag):"memory");
  }
}

void writer()
{
  //touch global flag for ever
  int i;
  int dummy;
  while(1){
    //    nr_op++;
    for(i=0;i<nr_loop;i++)
      __asm__ volatile("rep; nop\n\t"::: "memory");

    asm volatile("movl $1, %2\n\t"
		 "movl $1, %1\n\t"
		 "incl %0\n\t"
		 :"+m"(nr_op),"+m"(dummy),"+m"(flag)::"memory");

  }
}

#define READMODE (0)
#define WRITEMODE (1)

void *dummythread(void *par)
{
  unsigned long cmd = (unsigned long)par;
  unsigned int mode = cmd & (0xffffffff);
  unsigned int cpu = cmd >> 32;
  fprintf(stderr,"dummythread in mode %d at cpu %d\n", mode, cpu);
  int i = 0;
  gotocpu(cpu);
  if (mode == READMODE)
    writer();
  else
    reader();
}

#define ARGV_CMD_OFFSET (1)
#define ARGV_CMD_NRTHREAD (2)
#define ARGV_CMD_NRPAYLOAD (3)
#define ARGV_CMD_NRLOOP (4)
#define ARGV_CMD_AFFINITY (5)

int
main(int argc, char **argv)
{
  int i = 0;
  int nr_thread = 0;
  pthread_t *threads;
  pthread_attr_t *attrs;

  if (argc < 4){
    fprintf(stderr,"threadrd read/write nrthread nrpayload nrloop affinity1 affinity2...\n");
    exit(0);
  }

  int mode = WRITEMODE;
  if (strcmp(argv[ARGV_CMD_OFFSET], "read") == 0)
    mode = READMODE;
  else if(strcmp(argv[ARGV_CMD_OFFSET], "write") == 0)
    mode = WRITEMODE;
  else{
    fprintf(stderr,"unknow mode %s\n", argv[1]);
    exit(0);
  }

  nr_thread = atoi(argv[ARGV_CMD_NRTHREAD]);
  nr_loop = atoi(argv[ARGV_CMD_NRLOOP]);
  nr_payload = atoi(argv[ARGV_CMD_NRPAYLOAD]);

  fprintf(stderr,"Main thread go to cpu 0\n");
  gotocpu(0);

  if (nr_thread != 0){
    threads = (pthread_t *) malloc(sizeof(pthread_t) * nr_thread);
    attrs = (pthread_attr_t *)malloc(sizeof(pthread_attr_t) * nr_thread);
  }
  for(i=0;i<nr_thread;i++){
    unsigned long cpu = atoi(argv[ARGV_CMD_AFFINITY+i]);
    fprintf(stderr,"create %d worker thread on cpu %d\n", i, (int) cpu);
    pthread_attr_init(attrs + i);
    pthread_create(threads + i, attrs + i, &dummythread, (void *)((cpu << 32 )| mode));
  }

  //create counter here

  sleep(1);

  sysPerfEventInit(1);

  sysPerfEventCreate(0, events[0]);

  //  sysPerfEventEnable();

  printf("{\n\"items\":[\n");
  long long perf_begin[3];
  long long perf_end[3];

  for (i=0; i<20; i=i+1){
    sysPerfEventRead(0, perf_begin);
    long begin = rdtsc();
    unsigned long start_op = nr_op;
    long retval = 0;
    if (mode == READMODE)
      readflag(10000);
    else
      writeflag(10000);
    long end = rdtsc();
    sysPerfEventRead(0, perf_end);

    unsigned long end_op = nr_op;
    unsigned int cycles = end - begin;
    float cycles_per_op = (float)cycles /10000;
    unsigned int worker_ops = end_op - start_op;
    float cycles_per_workerop = 0;
    if (worker_ops !=0)
      cycles_per_workerop = (float)cycles / worker_ops;

    if (i!=0){
      printf(",");
    }
    printf("{\"iter\":%d, \"nrwrite\":%d, \"cycles\":%d, \"cpw\":%f, \"nrread\":%d, \"cpr\":%f, \"event\":%lld}\n", i, 10000, cycles, cycles_per_op, worker_ops, cycles_per_workerop, perf_end[0] - perf_begin[0]);
  }
  printf("]}\n");
}
