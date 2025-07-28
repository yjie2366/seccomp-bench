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
static int num_runs = 1000;
static double *time_each = NULL;

#define BIND_CPU(c) do {\
	int ret; cpu_set_t set;\
	CPU_ZERO(&set); CPU_SET(c, &set);\
	fprintf(stderr, "Bind to CPU %d\n", c);\
	ret = sched_setaffinity(getpid(), sizeof(set), &set);\
	if (ret == -1) { perror("sched_setaffinity failed: "); return -1; }\
} while (0)

static inline int init_bench(int argc, char **argv) {
	int cpu = 0; // default

	/* TODO: need to check error here */
	if (argc > 1) {
		cpu = strtol(argv[1], NULL, 10);
	}
	BIND_CPU(cpu);

	hz = tick_helz(0);

	time_each = calloc(num_runs, sizeof(double));
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

	for (i = 0; i < num_runs; i++) {
		total += time_each[i];	
	}
	return total/num_runs;
}

#endif
