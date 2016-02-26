#!/bin/bash
#
TEST="Test apcitables against invalid HEST"
NAME=test-0001.sh
TMPLOG=$TMP/hest.log.$$

$FWTS --show-tests | grep HEST > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/hest-0001/acpidump-0002.log hest - | cut -c7- | grep "^hest" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/hest-0001/hest-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
