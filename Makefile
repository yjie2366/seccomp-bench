SYSCALLS = getuid ioctl openat
BIN_ORG = $(foreach f, $(SYSCALLS), $(f)_org)
BIN_SEC = $(foreach f, $(SYSCALLS), $(f)_sec)
CFLAGS = -Wall -O2
SECCOMP_LIB = -lseccomp

.PHONY: all clean

all: $(BIN_ORG) $(BIN_SEC) openat_notify

%_org: test-%.c
	gcc $(CFLAGS) $^ -o $@ 

%_sec: test-%.c
	gcc $(CFLAGS) -DHAVE_SECCOMP $^ -o $@ $(SECCOMP_LIB)

openat_notify: test-openat.c
	gcc $(CFLAGS) -DHAVE_SECCOMP -DMEASURE_NOTIFY $^ -o $@ $(SECCOMP_LIB)

clean:
	rm -f $(BIN_ORG) $(BIN_SEC)
