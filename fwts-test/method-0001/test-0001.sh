#!/bin/bash
#
TEST="Test method against known correct ACPI tables"
NAME=test-0001.sh
TMPLOG=$TMP/method.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=acpidump-0001.log method - | grep "^[0-9]*[ ]*method" | cut -c7- > $TMPLOG
diff $TMPLOG method-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
