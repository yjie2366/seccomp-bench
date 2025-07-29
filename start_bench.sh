#!/bin/bash

cpu=(0 1)
freq=1800000
governor="userspace"
syscalls=(getuid ioctl)

for c in ${cpu[@]}; do
	echo ${governor} > /sys/devices/system/cpu/cpu${c}/cpufreq/scaling_governor
	echo ${freq} > /sys/devices/system/cpu/cpu${c}/cpufreq/scaling_setspeed
	cat /sys/devices/system/cpu/cpu${c}/cpufreq/scaling_governor
done


# common options
systemd_cmd="systemd-run --slice=benchmark.slice --property=AllowedCPUs=${cpu[0]} --wait"
# for getuid and ioctl
for syscall in "${syscalls[@]}"; do
	${systemd_cmd} --unit=${syscall}_org --property=StandardError=file:`pwd`/org_${syscall}.txt  ./${syscall}_org
	${systemd_cmd} --unit=${syscall}_sec --property=StandardError=file:`pwd`/sec_${syscall}.txt ./${syscall}_sec
done

# for openat
systemd_cmd_op="systemd-run --slice=benchmark.slice --property=AllowedCPUs=0,1 --wait"
${systemd_cmd_op} --unit=openat_org --property=StandardError=file:`pwd`/org_openat.txt  ./openat_org
${systemd_cmd_op} --unit=openat_sec --property=StandardError=file:`pwd`/sec_openat.txt ./openat_sec

# openat_notify
${systemd_cmd_op} --unit=openat_notify --property=StandardError=file:`pwd`/notify_openat.txt  ./openat_notify

