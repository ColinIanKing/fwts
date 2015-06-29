#!/bin/bash
#
TEST="Test table against CPEP"
NAME=test-0001.sh
TMPLOG=$TMP/cpep.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/cpep-0001/acpidump-0001.log cpep - | cut -c7- | grep "^cpep" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/cpep-0001/cpep-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
