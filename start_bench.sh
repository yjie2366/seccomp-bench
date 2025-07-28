#!/bin/bash

cpu=0
freq=1800000
governor="userspace"

echo ${governor} > /sys/devices/system/cpu/cpu${cpu}/cpufreq/scaling_governor
echo ${freq} > /sys/devices/system/cpu/cpu${cpu}/cpufreq/scaling_setspeed

cat /sys/devices/system/cpu/cpu${cpu}/cpufreq/scaling_governor

./getuid-orig ${cpu} &> orig.txt
./getuid-sec ${cpu} &> sec.txt





