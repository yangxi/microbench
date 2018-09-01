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

#ifdef NUMA
#include <numaif.h>
#endif

#define PAGE_SIZE (4*1024)

static __inline uint64_t
rdtsc(void)
{
	u_int32_t low, high;

	__asm __volatile("rdtsc" : "=a" (low), "=d" (high));
	return (low | ((u_int64_t)high << 32));
}

char	*progname;
int	ncpu = 1;
int	msize = (32 * 1024 * 1024);
int	ticks = 1;
int	fflag;
int	gflag;
int     gnode = 0;
int	wflag;
int	rflag;
int	bsize;
int always = 0;
int affinity = 1;

#define	TICK_INTERVAL	1000000000		/* One billion ticks */
#define	BLOCK_MIN	64
#define	BLOCK_SIZE	4096
#define	roundup2(x, y)	(((x)+((y)-1))&(~((y)-1)))

typedef long mr_op(char *buf, int len, int iter);

struct mr_thread {
	pthread_t	mt_thr;
	mr_op		*mt_op;
	char		*mt_buf;
	uint64_t	mt_ticks;
	int		mt_len;
	int		mt_cpu;
	int		mt_icount;
	uint64_t	mt_iticks;
	uint64_t	mt_tticks;
	uint64_t	mt_tusec;
	uint64_t	mt_bytes;
};

/*
 *Mov the number of pages from base_addr to NUMA node
 */

#ifdef NUMA
int
mov_p(unsigned int node, void *base_addr, unsigned int nr_page)
{
  int status[nr_page];
  int nodes[nr_page];
  void *pages[nr_page];
  unsigned int i;

  printf("Mov_p\n");
  for(i=0;i<nr_page;i++){
    pages[i] = (void *)((unsigned char *)base_addr + i*PAGE_SIZE);
    nodes[i] = node;
  }

  if(move_pages(getpid(),nr_page,pages,nodes,status,MPOL_MF_MOVE) == -1){
    return -1;
  }

  for(i=0;i<nr_page;i++){
    if(status[i] != node){
      return -1;
    }
  }

  return 0;
}
#endif

int
memzeronti(void *dst, int len)
{

	/*
	 * amd64 abi says args order %rdi, %rsi, %rdx, %rcx, %r8, etc.
	 * callee saved are %rbp, %rbx, %r12 - %r15.
	 */
	__asm__ __volatile__(
		".align	16						\n\t"
		"64:							\n\t"
		"cmp	$64, %%rsi					\n\t"
		"jl 0f							\n\t"
		"mov	$0, %%rax					\n\t"
		"movnti	%%rax, 0x0(%%rdi)				\n\t"
		"movnti	%%rax, 0x8(%%rdi)				\n\t"
		"movnti	%%rax, 0x10(%%rdi)				\n\t"
		"movnti	%%rax, 0x18(%%rdi)				\n\t"
		"movnti	%%rax, 0x20(%%rdi)				\n\t"
		"movnti	%%rax, 0x28(%%rdi)				\n\t"
		"movnti	%%rax, 0x30(%%rdi)				\n\t"
		"movnti	%%rax, 0x38(%%rdi)				\n\t"
		"add	$64, %%rdi					\n\t"
		"sub	$64, %%rsi					\n\t"
		"jmp	64b						\n\t"
		"0:							\n\t"
		: "+S"(len): "D" ( dst ));

	return len;
}

int
memcpynti(void *dst, const void *src, int len)
{
	/*
	 * amd64 abi says args order %rdi, %rsi, %rdx, %rcx, %r8, etc.
	 * callee saved are %rbp, %rbx, %r12 - %r15.
	 */
	__asm__ __volatile__(
		"push	%%r12						\n\t"
		"push	%%r13						\n\t"
		".align	16						\n\t"
		"64:							\n\t"
		"cmp	$64, %%rdx					\n\t"
		"jl 0f							\n\t"
		"movq	0x0(%%rsi), %%rax				\n\t"
		"movnti	%%rax, 0x0(%%rdi)				\n\t"
		"movq	0x8(%%rsi), %%rcx				\n\t"
		"movnti	%%rcx, 0x8(%%rdi)				\n\t"
		"movq	0x10(%%rsi), %%r8				\n\t"
		"movnti	%%r8,  0x10(%%rdi)				\n\t"
		"movq	0x18(%%rsi), %%r9				\n\t"
		"movnti	%%r9,  0x18(%%rdi)				\n\t"
		"movq	0x20(%%rsi), %%r10				\n\t"
		"movnti	%%r10, 0x20(%%rdi)				\n\t"
		"movq	0x28(%%rsi), %%r11				\n\t"
		"movnti	%%r11, 0x28(%%rdi)				\n\t"
		"movq	0x30(%%rsi), %%r12				\n\t"
		"movnti	%%r12, 0x30(%%rdi)				\n\t"
		"movq	0x38(%%rsi), %%r13				\n\t"
		"movnti	%%r13, 0x38(%%rdi)				\n\t"
		"sub	$64, %%rdx					\n\t"
		"add	$64, %%rdi					\n\t"
		"add	$64, %%rsi					\n\t"
		"jmp	64b						\n\t"
		"0:							\n\t"
		"pop	%%r13						\n\t"
		"pop	%%r12						\n\t"
		: "+d"(src): "D" ( dst ) , "S" (src));

	return len;
}

static long
mr_write(char *buf, int len, int iter)
{
	char *dst;
	int block;

	block = iter % (len / bsize);
	dst = buf + (block * bsize);

	if (fflag)
		memzeronti(dst, bsize);
	else
		memset(dst, 0, bsize);

	return (bsize);
}

static long
mr_rw(char *buf, int len, int iter)
{
	char *dst;
	char *src;
	int block;

	block = iter % (len / bsize);
	src = buf + (block * bsize);
	block = (iter + 1) % (len / bsize);
	dst = buf + (block * bsize);
	if (fflag)
		memcpynti(dst, src, bsize);
	else
		memcpy(dst, src, bsize);

	return (bsize * 2);
}

static long
mr_compute(char *buf, int len, int iter)
{
}

static long
mr_read(char *buf, int len, int iter)
{
	long *src;
	long *end;
	int block;
	long sum;

	block = iter % (len / bsize);
	src = (long *)(buf + (block * bsize));
	end = src + (bsize / sizeof(long));
	sum = 0;
	/* Read and keep a sum so the compiler doesn't throw things away. */
	while (src < end){
	  asm volatile("movq %1,%0\n\t":"=r"(sum):"m"(*src));
	  src++;
	}
	  //sum += *src++;

	return (bsize);
}

static void
mr_alloc(struct mr_thread *mtp)
{
  int i;

	if (mtp->mt_buf == NULL)
		mtp->mt_buf = malloc(mtp->mt_len);
	if (mtp->mt_buf == NULL) {
		fprintf(stderr, "Failed to allocate %d bytes\n", mtp->mt_len);
		exit(EXIT_FAILURE);
	}
	/*Touch the page to move memory to local node */
	for(i=0;i<mtp->mt_len;i++)
	  mtp->mt_buf[i] = 0;
}

static void
mr_free(struct mr_thread *mtp)
{
  if (mtp->mt_buf)
    free(mtp->mt_buf);
}

static void
mr_bind(struct mr_thread *mtp)
{
	cpu_set_t mask;
	int cpu;

	cpu = mtp->mt_cpu;
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	if (sched_setaffinity(0, sizeof(cpu_set_t), &mask)) {
		fprintf(stderr, "CPU %d: sched_setaffinity: %s\n",
		    cpu, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

static void *
mr_thread(void *arg)
{
	struct timeval tv_begin;
	struct timeval tv_end;
	struct mr_thread *mtp;
	uint64_t start;
	uint64_t begin;
	uint64_t stop;
	uint64_t end;
	uint64_t now;
	int i;

	mtp = (struct mr_thread *)arg;
	if (affinity)
	  mr_bind(mtp);
	mr_alloc(mtp);

	do{
	/*
	 * We want to run for one tick interval before and after we sample
	 * so that we know all processors were spun up and running at the
	 * same time.  This overlap helps to ensure that.
	 */
	gettimeofday(&tv_begin, NULL);
	begin = rdtsc();
	start = begin + TICK_INTERVAL;
	stop = start + (mtp->mt_ticks * TICK_INTERVAL);
	end = stop + TICK_INTERVAL;
	/* Loop until we reach the start time. */
	for (i = 0; (now = rdtsc()) < start; i++)
		mtp->mt_op(mtp->mt_buf, mtp->mt_len, i);
	/*
	 * Record the real start time and loop until we reach stop, counting
	 * iterations.
	 */
	start = now;
	for (i = 0; (now = rdtsc()) < stop; i++)
		mtp->mt_op(mtp->mt_buf, mtp->mt_len, i);
	stop = now;
	mtp->mt_icount = i;
	/*
	 * Now loop for one more interval so we are certain we overlap
	 * with other threads.
	 */
	for (i = 0; rdtsc() < end; i++)
	  mtp->mt_bytes += mtp->mt_op(mtp->mt_buf, mtp->mt_len, i);

	mtp->mt_iticks = stop - start;
	mtp->mt_tticks = rdtsc() - begin;
	gettimeofday(&tv_end, NULL);
	/* Now re-use begin/end to calculate microseconds of runtime */
	begin = tv_begin.tv_sec * 1000000 + tv_begin.tv_usec;
	end = tv_end.tv_sec * 1000000 + tv_end.tv_usec;
	mtp->mt_tusec = end - begin;
	} while(always);

	mr_free(mtp);

	pthread_exit(NULL);
}

char *
mr_prefix(uint64_t bytes, float *bytep)
{
	char *prefixes[] = { "b", "Kb", "Mb", "Gb", "Tb", NULL };
	char **prefix;
	float b;

	b = (float)bytes;
	prefix = prefixes;
	while (b > 1024) {
		b /= 1024;
		prefix++;
	}
	*bytep = b;
	return (*prefix);
}

static void
mr_report(struct mr_thread *tds)
{
	uint64_t intervals;
	uint64_t runus;
	uint64_t ticks;
	uint64_t usec;
	uint64_t tticks;
	uint64_t bytes;
	uint64_t tpus;
	float hbytes;
	char *p;
	int i;

	/* Aggregate results */
	usec = tticks = ticks = intervals = bytes = 0;
	for (i = 0; i < ncpu; i++) {
		ticks += tds[i].mt_iticks;
		intervals += tds[i].mt_icount;
		tticks += tds[i].mt_tticks;
		usec += tds[i].mt_tusec;
		bytes += tds[i].mt_bytes;
	}
	/*
	 * Convert to averages.
	 */
	usec /= ncpu;
	tticks /= ncpu;
	ticks /= ncpu;
	intervals /= ncpu;
	bytes /= ncpu;
	/*
	 * Total ticks per usec.
	 */
	tpus = tticks / usec;
	/*
	 * Number of microseconds we were running for.
	 */
	runus = ticks / tpus;
	p = mr_prefix(bytes, &hbytes);
	/*
	 * Output some results.
	 */

	printf("Average of %ju ticks and %ju intervals for %.2f%s\n",
	    ticks, intervals, hbytes, p);
	printf("Total usecs %ju for %ju ticks\n", usec, tticks);
	printf("Ticks per usec %ju\n", tpus);

	bytes /= runus;
	p = mr_prefix(bytes, &hbytes);
	printf("Running time %ju usec, bytes per usec: %0.f%s\n",
	    runus, hbytes, p);
	bytes *= 1000000;
	p = mr_prefix(bytes, &hbytes);
	printf("Bytes per second per cpu: %.2f%s\n", hbytes, p);
	bytes *= ncpu;
	p = mr_prefix(bytes, &hbytes);
	printf("Total bytes per second: %.2f%s\n", hbytes, p);
}

static void
mr_start(mr_op *op)
{
	struct mr_thread tds[ncpu];
	pthread_attr_t attr;
	char *buf;
	int i;

	memset(&tds, 0, sizeof(tds));
	pthread_attr_init(&attr);
	if (gflag) {
		buf = malloc(msize);
		if (buf == NULL) {
			fprintf(stderr, "Failed to allocate %d bytes\n", msize);
			exit(EXIT_FAILURE);
		}
		/*Touch the global memory*/
		for(i=0;i<msize;i++)
		  buf[i] = 0;
#ifdef NUMA
		if(mov_p(gnode,buf,msize/PAGE_SIZE) == -1)
		  printf("Can not mov buf to node %d\n",gnode);
#endif
	} else
		buf = NULL;

	for (i = 0; i < ncpu; i++) {
		tds[i].mt_cpu = i;
		tds[i].mt_buf = buf;
		tds[i].mt_len = msize;
		tds[i].mt_ticks = ticks;
		tds[i].mt_op = op;
		if (pthread_create(&tds[i].mt_thr, &attr, mr_thread,
		    &tds[i])) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}
	for (i = 0; i < ncpu; i++)
		pthread_join(tds[i].mt_thr, NULL);
	mr_report(tds);
}

static void
usage()
{
	fprintf(stderr, "usage: %s [-fgrw -m size -n cpu -t ticks]\n",
	    progname);
	fprintf(stderr, "\t-a Use running forever\n");
	fprintf(stderr, "\t-b do not set affinity\n");
	fprintf(stderr, "\t-f Use \'fast\' non-temporal store functions\n");
	fprintf(stderr, "\t-g Use a global memory buffer for all CPUs\n");
	fprintf(stderr, "\t-r Read memory.  May be combined with -w\n");
	fprintf(stderr, "\t-w Write memory.  May be combined with -r\n");
	fprintf(stderr, "\t-m Size of buffer in bytes, per-cpu or global.\n");
	fprintf(stderr, "\t-n Number of cpus to execute on concurrently.\n");
	fprintf(stderr, "\t-t Number of cycles to execute for x 1 billion\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	mr_op *op;
	int ch;

	progname = argv[0];
	while ((ch = getopt(argc, argv, "abfg:m:n:rt:w")) != -1) {
		switch (ch) {
		case 'a':
		  always = 1;
		  break;
		case 'b':
		  affinity = 0;
		  break;
		case 'f':
			fflag = 1;
			break;
		case 'g':
			gflag = 1;
			gnode = atoi(optarg);
			break;
		case 'm':
			msize = roundup2(atoi(optarg), BLOCK_MIN*2);
			break;
		case 'n':
			ncpu = atoi(optarg);
			break;
		case 'r':
			rflag = 1;
			break;
		case 't':
			ticks = atoi(optarg);
			break;
		case 'w':
			wflag = 1;
			break;
		default:
			usage();
		}
	}
	/* Both are set or default to zero use mr_rw. */
	if (rflag == wflag)
		op = mr_rw;
	else if (rflag)
		op = mr_read;
	else
		op = mr_write;
	printf("rflag,wflag=%d,%d\n",rflag,wflag);
	if (msize < BLOCK_SIZE)
		bsize = msize / 2;	/* space for two blocks for memcpy */
	else
		bsize = BLOCK_SIZE;

	printf("Number of CPUS: %d, Memory size: %d, Billion ticks: %d\n",
	    ncpu, msize, ticks);
	while(1){
	  mr_start(op);
	  if (always == 0)
	    break;
	}
	exit(EXIT_SUCCESS);
}
