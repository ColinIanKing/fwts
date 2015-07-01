#!/bin/bash
#
TEST="Test table against SLIC"
NAME=test-0001.sh
TMPLOG=$TMP/slic.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/slic-0001/acpidump-0001.log slic - | cut -c7- | grep "^slic" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/slic-0001/slic-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
