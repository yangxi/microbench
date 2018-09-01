#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <sched.h>
#include <sys/sysinfo.h>
#include <perfmon/pfmlib_perf_event.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

struct shim_hardware_event {
  int index;
  int fd;
  struct perf_event_attr perf_attr;
  struct perf_event_mmap_page *buf;
  char * name;
};

static uint64_t __inline__ rdtsc(void)
{
  unsigned int tickl, tickh;
  __asm__ __volatile__("rdtscp":"=a"(tickl),"=d"(tickh)::"%ecx");
  return ((uint64_t)tickh << 32)|tickl;
}

void shim_init()
{
  //from libpfm library man page, http://perfmon2.sourceforge.net/manv4/pfm_initialize.html
  //This is the first function that a program must call otherwise the library will not function at all.
  //This function probes the underlying hardware looking for valid PMUs event tables to activate.
  //Multiple distinct PMU tables may be activated at the same time.The function must be called only once.
  int ret = pfm_initialize();
  if (ret != PFM_SUCCESS) {
    err(1,"pfm_initialize() is failed!");
    exit(-1);
  }
}

void shim_thread_init(int cpuid, int nr_hw_events, const char **hw_event_names)
{
  int i;
  debug_print("init shim thread at cpu %d\n", cpuid);
  bind_processor(cpuid);
  my->cpuid = cpuid;
  my->nr_hw_events = nr_hw_events;
  my->hw_events = (struct shim_hardware_event *)calloc(nr_hw_events, sizeof(struct shim_hardware_event));
  //  assert(my->hw_events != NULL);
  for (i=0; i<nr_hw_events; i++){
    shim_create_hw_event(hw_event_names[i], i, my);
  }
  for (i=0;i <nr_hw_events; i++){
    struct shim_hardware_event *e = my->hw_events + i;
    debug_print("updateindex event %s, fd %d, index %x\n", e->name, e->fd, e->buf->index - 1);
    e->index = e->buf->index - 1;

  }
  my->probe_other_events = NULL;
  my->probe_tags = NULL;
}




static void shim_create_hw_event(char *name, int id, shim *myshim)
{
  struct shim_hardware_event * event = myshim->hw_events + id;
  struct perf_event_attr *pe = &(event->perf_attr);
  int ret = pfm_get_perf_event_encoding(name, PFM_PLM3, pe, NULL, NULL);
  if (ret != PFM_SUCCESS) {
    errx(1, "error creating event %d '%s': %s\n", id, name, pfm_strerror(ret));
  }
  pe->sample_type = PERF_SAMPLE_READ;
  event->fd = perf_event_open(pe, 0, -1, -1, 0);
  if (event->fd == -1) {
    err(1, "error in perf_event_open for event %d '%s'", id, name);
  }
  //mmap the fd to get the raw index
  event->buf = (struct perf_event_mmap_page *)mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ, MAP_SHARED, event->fd, 0);
  if (event->buf == MAP_FAILED) {
    err(1,"mmap on perf fd");
  }

  event->name = copy_name(name);

  event->index = event->buf->index - 1;
  debug_print("SHIM %d:creat %d hardware event name:%s, fd:%d, index:%x\n",
	      myshim->cpuid,
	      id,
	      name,
	      event->fd,
	      event->index);
}

int shim_read_counters(uint64_t *buf, shim *myshim)
{
  //[0] and [1] are start timestamp and end timestamp
  int index = 2;
  int i = 0;
  //start timestamp
  buf[0] = rdtsc();
  //hardware counters
  for (i=0; i<myshim->nr_hw_events; i++){
    rdtsc();
    buf[index++] = __builtin_ia32_rdpmc(myshim->hw_events[i].index);
  }
  //call back probe_other_events is happened between getting two timestamps
  if (myshim->probe_other_events != NULL)
    index += myshim->probe_other_events(buf + index, myshim);
  //end timestamp
  buf[1] = rdtsc();
  //call back probe_other_tags is happend after reading two timestamps
  if (myshim->probe_tags != NULL)
    index += myshim->probe_tags(buf + index, myshim);
  return index;
}

int
main(int argc, char **argv)
{
  //create the counter


}
