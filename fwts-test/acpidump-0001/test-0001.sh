#!/bin/bash
#
TEST="Test acpidump against known ACPI tables"
NAME=test-0001.sh
TMPLOG=$TMP/acpidump.log.$$

$FWTS --show-tests | grep acpidump  > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/acpidump-0001/acpidump.log acpidump - | grep "^[0-9]*[ ]*acpidump" | cut -c7- > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/acpidump-0001/acpidump-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
