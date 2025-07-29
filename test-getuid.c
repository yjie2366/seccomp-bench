#include "test.h"

int main(int argc, char **argv)
{
	int ret = 0;
	uid_t uid = 0;

	ret = init_bench(argc, argv);
	if (ret) {
		fprintf(stderr, "Failed to initialize bench\n");
		goto out;
	}

#ifdef HAVE_SECCOMP
	scmp_filter_ctx ctx;

	ctx = seccomp_init(SCMP_ACT_LOG);
	if (!ctx) {
		perror("seccomp_init failed: ");
		ret = -1;
		goto out;
	}

	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, getuid, 0);
	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, write, 0);
	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, exit_group, 0);

	ret = seccomp_load(ctx);
	if (ret) {
		fprintf(stderr, "Failed to load filter: %s\n", strerror(errno));
		goto out;
	}
#endif

	int i;
	uint64_t st = 0, end = 0;

	// warmup
	for (i = 0; i < NUM_WARMUP; i++) {
		uid = syscall(__NR_getuid);
	}

	for (i = 0; i < num_runs; i++) {
		st = tick_time();
		uid = syscall(__NR_getuid);
		end = tick_time();
		time_each[i] = (double)(end - st)/(double)hz * 1000000000;

		fprintf(stderr, "UID: %d RUN %d: %12.2fns\n",
			uid, i, time_each[i]);
	}

	fprintf(stderr, "AVG: %12.2fns\n", average_time());

#ifdef HAVE_SECCOMP
	seccomp_release(ctx);
#endif

out:
	finalize_bench();
	return ret;
}
