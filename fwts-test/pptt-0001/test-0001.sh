#!/bin/bash
#
TEST="Test acpitables against PPTT"
NAME=test-0001.sh
TMPLOG=$TMP/pptt.log.$$

$FWTS --show-tests | grep pptt > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/pptt-0001/acpidump-0001.log pptt - | cut -c7- | grep "^pptt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/pptt-0001/pptt-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
