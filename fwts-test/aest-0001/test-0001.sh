#!/bin/bash
#
TEST="Test acpitables against AEST"
NAME=test-0001.sh
TMPLOG=$TMP/aest.log.$$

$FWTS --show-tests | grep aest > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/aest-0001/acpidump-0001.log aest - | cut -c7- | grep "^aest" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/aest-0001/aest-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
