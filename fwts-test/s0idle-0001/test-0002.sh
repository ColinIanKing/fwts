#!/bin/bash
#
TEST="Test table against invalid S0IDLE"
NAME=test-0001.sh
TMPLOG=$TMP/s0idle.log.$$

$FWTS --show-tests | grep S0IDLE > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/s0idle-0001/acpidump-0002.log s0idle - | cut -c7- | grep "^s0idle" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/s0idle-0001/s0idle-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
