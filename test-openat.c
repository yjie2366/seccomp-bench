#include "test.h"
#include <fcntl.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
	int ret = 0, i;

	ret = init_bench(argc, argv);
	if (ret) {
		fprintf(stderr, "Failed to initialize bench\n");
		goto out;
	}

#ifdef HAVE_SECCOMP
	int noti_fd = -1; pid_t pid = 0;
	scmp_filter_ctx ctx;

	ret = bind_cpu(1);
	if (ret) {
		fprintf(stderr, "bind_cpu(1) failed\n");
		goto out;
	}

	ctx = seccomp_init(SCMP_ACT_LOG);
	if (!ctx) {
		perror("seccomp_init failed");
		ret = -1;
		goto out;
	}

	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_NOTIFY, openat, 0);
	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, write, 0);
	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, seccomp, 0);
	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, ioctl, 0);
	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, getpid, 0);
	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, sched_setaffinity, 0);
	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, clone, 0);
	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, close, 0);
	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, wait4, 0);
	CREATE_SECCOMP_RULE(ctx, SCMP_ACT_ALLOW, exit_group, 0);

	ret = seccomp_load(ctx);
	if (ret) {
		fprintf(stderr, "seccomp_load faild: %s\n", strerror(errno));
		goto out;
	}

	noti_fd = seccomp_notify_fd(ctx);
	if (noti_fd < 0) {
		fprintf(stderr, "seccomp_notify_fd failed: %s\n", strerror(errno));
		ret = -1;
		goto out;
	}

	pid = fork();
	if (pid) {
		struct seccomp_notif *req = NULL;
		struct seccomp_notif_resp *resp = NULL;

		ret = seccomp_notify_alloc(&req, &resp);
		if (ret) {
			perror("seccomp_notify_alloc failed");
			goto out;
		}

		for (i = -NUM_WARMUP; i < NUM_RUNS; i++) {
			ret = seccomp_notify_receive(noti_fd, req);
			if (ret) {
				perror("seccomp_notify_receive failed");
				goto out;
			}
#ifdef MEASURE_NOTIFY
if (i >= 0)
	fprintf(stderr, "RUN %d: received %ld\n", i, tick_time());
#endif
			ret = seccomp_notify_id_valid(noti_fd, req->id);
			if (ret) {
				perror("seccomp_notify_id_valid failed");
				goto out;
			}

			resp->id = req->id;
			resp->flags = SECCOMP_USER_NOTIF_FLAG_CONTINUE;

			ret = seccomp_notify_respond(noti_fd, resp);
			if (ret) {
				perror("seccomp_notify_respond failed");
				goto out;
			}

			memset(req, 0, sizeof(struct seccomp_notif));
			memset(resp, 0, sizeof(struct seccomp_notif_resp));
		}

		seccomp_notify_free(req, resp);
	}
	else if (!pid) {
#endif
	ret = bind_cpu(2);
	if (ret) {
		fprintf(stderr, "bind_cpu(2) failed\n");
		goto out;
	}

	int fd = -1;
	char *filename = "/dev/null";
#ifndef MEASURE_NOTIFY
	uint64_t st = 0, end = 0;
#endif

	// warmup
	for (i = 0; i < NUM_WARMUP; i++) {
		fd = syscall(__NR_openat, AT_FDCWD, filename, O_RDONLY);
		if (fd == -1) {
			perror("openat failed");
			ret = -1;
			goto out;
		}
		close(fd);
	}

	for (i = 0; i < NUM_RUNS; i++) {
#ifdef MEASURE_NOTIFY
fprintf(stderr, "RUN %d: start %ld\n", i, tick_time());
#else
		st = tick_time();
#endif
		fd = syscall(__NR_openat, AT_FDCWD, filename, O_RDONLY);
		if (fd == -1) {
			perror("openat failed");
			ret = -1;
			goto out;
		}
#ifdef MEASURE_NOTIFY
fprintf(stderr, "RUN %d: end %ld\n", i, tick_time());
#else
		end = tick_time();
#endif
		close(fd);

#ifndef MEASURE_NOTIFY
		time_each[i] = (double)(end - st)/(double)hz * 1000000000;
		fprintf(stderr, "RUN %d: %12.2fns\n", i, time_each[i]);
	}

	fprintf(stderr, "AVG: %12.2fns\n", average_time());
#else
	}
#endif

	exit(0);

#ifdef HAVE_SECCOMP
	} else {
		perror("fork failed");
		ret = -1;
	}

	waitpid(pid, NULL, 0);
	seccomp_release(ctx);
#endif

out:
	finalize_bench();
	return ret;
}
