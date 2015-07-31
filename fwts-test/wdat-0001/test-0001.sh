#!/bin/bash
#
TEST="Test apci table against WDAT"
NAME=test-0001.sh
TMPLOG=$TMP/wdat.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/wdat-0001/acpidump-0001.log wdat - | cut -c7- | grep "^wdat" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/wdat-0001/wdat-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
