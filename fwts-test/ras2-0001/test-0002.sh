#!/bin/bash
#
TEST="Test acpitables against RAS2"
NAME=test-0002.sh
TMPLOG=$TMP/ras2.log.$$

$FWTS --show-tests | grep sdev > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/ras2-0001/acpidump-0002.log ras2 - | cut -c7- | grep "^ras2" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/ras2-0001/ras2-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
