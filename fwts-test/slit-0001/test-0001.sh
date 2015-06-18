#!/bin/bash
#
TEST="Test apcitables against SLIT"
NAME=test-0001.sh
TMPLOG=$TMP/slit.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/slit-0001/acpidump-0001.log slit - | cut -c7- | grep "^slit" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/slit-0001/slit-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
