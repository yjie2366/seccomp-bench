#include "test.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/videodev2.h>

int main(int argc, char **argv)
{
	int ret = 0;
	struct v4l2_capability cap = { 0 };
	int fd = -1;

	ret = init_bench(argc, argv);
	if (ret) {
		fprintf(stderr, "Failed to initialize bench\n");
		goto out;
	}

	fd = open("/dev/video0", O_RDWR);
	if (fd < 0) {
		perror("Failed to open device");
		ret = -1;
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

	ret = seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(ioctl), 1,
		SCMP_A1(SCMP_CMP_EQ, VIDIOC_QUERYCAP));
	if (ret) {
		fprintf(stderr, "Failed to add rule: %s for syscall: %s\n",
			strerror(errno), "ioctl");
		goto out;
	}

	ret = seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
	if (ret) {
		fprintf(stderr, "Failed to add rule: %s for syscall: %s\n",
			strerror(errno), "write");
		goto out;
	}

	ret = seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 0);
	if (ret) {
		fprintf(stderr, "Failed to add rule: %s for syscall: %s\n",
			strerror(errno), "close");
		goto out;
	}

	ret = seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
	if (ret) {
		fprintf(stderr, "Failed to add rule: %s for syscall: %s\n",
			strerror(errno), "exit_group");
		goto out;
	}

	ret = seccomp_load(ctx);
	if (ret) {
		fprintf(stderr, "Failed to load filter: %s\n", strerror(errno));
		goto out;
	}
#endif

	int i;
	uint64_t st = 0, end = 0;

	// warmup
	for (i = 0; i < 10; i++) {
		ret = syscall(__NR_ioctl, fd, VIDIOC_QUERYCAP, &cap);
		if (ret == -1) {
			perror("ioctl failed:");
			goto out;
		}
	}

	for (i = 0; i < num_runs; i++) {
		st = tick_time();

		ret = syscall(__NR_ioctl, fd, VIDIOC_QUERYCAP, &cap);
		if (ret == -1) {
			perror("ioctl failed:");
			goto out;
		}

		end = tick_time();
		time_each[i] = (double)(end - st)/(double)hz * 1000000000;

		fprintf(stderr, "RUN %d: %12.2fns\n", i, time_each[i]);
	}

	fprintf(stderr, "AVG: %12.2fns\n", average_time());

#ifdef HAVE_SECCOMP
	seccomp_release(ctx);
#endif

out:
	if (fd > 0) close(fd);
	finalize_bench();
	return ret;
}
