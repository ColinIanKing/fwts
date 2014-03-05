#!/bin/bash
#
TEST="Test osilinux against known incorrect ACPI tables"
NAME=test-0001.sh
TMPLOG=$TMP/osilinux.log.$$

$FWTS --show-tests | grep osilinux > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/osilinux-0001/acpidump-0001.log osilinux - | grep "^[0-9]*[ ]*osilinux" | cut -c7- > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/osilinux-0001/osilinux-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
