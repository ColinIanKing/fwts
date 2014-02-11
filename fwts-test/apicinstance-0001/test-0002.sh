#!/bin/bash
#
TEST="Test apicinstance against known correct ACPI tables"
NAME=test-0002.sh
TMPLOG=$TMP/apicinstance.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=acpidump-0002.log apicinstance - | grep "^[0-9]*[ ]*apicinstance" | cut -c7- > $TMPLOG
diff $TMPLOG apicinstance-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
