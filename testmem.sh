#!/bin/bash
nrCPU=`cat /proc/cpuinfo | grep processor | wc -l`
echo "there are $nrCPU in the system"

min=`cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq`
max=`cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq`
steps=`cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies`
for freq in $steps; do
    if (($freq > $max)); then
	echo "too large, try next one";
	continue
    fi
    ./setfreq.sh $nrCPU $min $max $freq
    rm ./$freq
    echo -e "0\t${freq}Core\t${freq}Total" > ./$freq
    for ((i=1; i<=$nrCPU; i=i+1)); do
	./memrate -r -n $i > ./tmp
	percore=`cat ./tmp | grep 'per cpu' | grep -o '[0-9.]*'`
	total=`cat ./tmp | grep 'Total bytes' | grep -o '[0-9.]*'`
	echo -e "${i}\t${percore}\t${total}"
	echo -e "${i}\t${percore}\t${total}" >> ./$freq
    done
done

./setfreq.sh $nrCPU $min $max $max
paste -d'    ' $steps > ./memratereadfreq.dat
