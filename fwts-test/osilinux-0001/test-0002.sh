#!/bin/bash
#
TEST="Test osilinux against known correct ACPI tables"
NAME=test-0002.sh
TMPLOG=$TMP/osilinux.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=acpidump-0002.log osilinux - | grep "^[0-9]*[ ]*osilinux" | cut -c7- > $TMPLOG
diff $TMPLOG osilinux-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
