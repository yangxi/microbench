#!/bin/bash
#usage setfreq nrCPU min max set
nrCPU=$1
min=$2
max=$3
freq=$4
echo "set all $nrCPU cpus freqncy to $freq (min $min max $max)"
for ((i=0;i<$nrCPU ;i=i+1)); do
    cpufreq-set -c $i -d $min -u $max
    cpufreq-set -c $i -f $freq
done

cpufreq-info | grep 'current CPU frequency' | uniq