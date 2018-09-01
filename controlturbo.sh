#!/bin/bash
#controlturbo on
#controlturbo off
#bit 38 -> turnbomode [0->on 1->off]
val=`sudo rdmsr 0x1A0`
echo $val
let "flag=0x$val & (1<<38)"
echo $flag
if [ $flag == 0 ]
then
    echo "turbo mode is on"
    if [ $1 == off ]
    then
	echo "turn it off"
	let "val=0x$val ^ (1<<38)"
	echo $val
	printf "val is %llx\n", $val
	sudo wrmsr 0x1A0 $val
    fi
else
    echo "turbo mode is off"
    if [ $1 == on ]
    then
	echo "turn if on"
	let "val=0x$val ^ (1<<38)"
	printf "val is %llx\n", $val
    fi
fi
