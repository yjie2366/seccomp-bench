#ifndef _TEST_H_
#define _TEST_H_

#define _GNU_SOURCE
#include <sched.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <seccomp.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "tsc.h"

static uint64_t hz = 0;
static double *time_each = NULL;

#define NUM_WARMUP 10
#define NUM_RUNS 10000

#define CREATE_SECCOMP_RULE(_ctx, ac, s, ...) do {\
	ret = seccomp_rule_add(_ctx, ac, SCMP_SYS(s), ##__VA_ARGS__);\
	if (ret) {\
		fprintf(stderr, "ERROR: add rule for syscall: %s (%s)\n", \
			#s, strerror(errno));\
		goto out;\
	}\
} while (0)

static inline int bind_cpu(int cpu)
{
	int ret = 0; cpu_set_t set;

	CPU_ZERO(&set);
	CPU_SET(cpu, &set);

	fprintf(stderr, "Bind to CPU %d\n", cpu);

	ret = sched_setaffinity(getpid(), sizeof(set), &set);
	if (ret == -1) {
		perror("sched_setaffinity failed: ");
		return -1;
	}
	return 0;
}

static inline int init_bench(int argc, char **argv)
{
	hz = tick_helz(0);

	time_each = calloc(NUM_RUNS, sizeof(double));
	if (!time_each) {
		perror("calloc() failed: ");
		return -1;
	}
	return 0;
}

static inline void finalize_bench(void)
{
	if (time_each) free(time_each);
}

static inline double average_time(void)
{
	int i;
	double total = 0.0;

	for (i = 0; i < NUM_RUNS; i++) {
		total += time_each[i];	
	}
	return total/NUM_RUNS;
}

#endif
