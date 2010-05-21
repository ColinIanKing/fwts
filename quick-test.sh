#!/bin/bash

LOG=results.log

ARGS="--results-output=$LOG --results-no-separators --stdout-summary"

tests="./src/acpi/klog/klog ./src/acpi/syntaxcheck/syntaxcheck ./src/dmi/dmi_decode/dmi_decode ./src/acpi/wakealarm/wakealarm"

echo "Results being dumped into $LOG"
rm -f $LOG

for I in $tests
do
	echo Test: `basename $I`
	sudo $I $ARGS
	echo " " >> $LOG
	echo " "
done
