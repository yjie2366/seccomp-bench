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

	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, ioctl, 2,
		SCMP_A0(SCMP_CMP_EQ, fd),
		SCMP_A1(SCMP_CMP_EQ, VIDIOC_QUERYCAP));
	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, write, 0);
	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, close, 0);
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
		ret = syscall(__NR_ioctl, fd, VIDIOC_QUERYCAP, &cap);
		if (ret == -1) {
			perror("ioctl failed:");
			goto out;
		}
	}

	for (i = 0; i < NUM_RUNS; i++) {
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
