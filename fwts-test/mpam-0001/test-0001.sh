#!/bin/bash
#
TEST="Test acpitables against MPAM"
NAME=test-0001.sh
TMPLOG=$TMP/mpam.log.$$

$FWTS --show-tests | grep mpam > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/mpam-0001/acpidump-0001.log mpam - | cut -c7- | grep "^mpam" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/mpam-0001/mpam-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
