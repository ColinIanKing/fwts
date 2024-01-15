#!/bin/bash
#
TEST="Test acpitables against SKVL"
NAME=test-0002.sh
TMPLOG=$TMP/skvl.log.$$

$FWTS --show-tests | grep skvl > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/skvl-0001/acpidump-0002.log skvl - | cut -c7- | grep "^skvl" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/skvl-0001/skvl-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
