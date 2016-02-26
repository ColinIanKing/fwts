#!/bin/bash
#
TEST="Test table against FPDT"
NAME=test-0001.sh
TMPLOG=$TMP/fpdt.log.$$

$FWTS --show-tests | grep FPDT > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/fpdt-0001/acpidump-0001.log fpdt - | cut -c7- | grep "^fpdt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/fpdt-0001/fpdt-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
