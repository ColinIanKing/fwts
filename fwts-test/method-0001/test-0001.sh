#!/bin/bash
#
TEST="Test method against known correct ACPI tables"
NAME=test-0001.sh
TMPLOG=$TMP/method.log.$$

$FWTS --show-tests | grep method > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/method-0001/acpidump-0001.log method - | grep "^[0-9]*[ ]*method" | cut -c7- > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/method-0001/method-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
