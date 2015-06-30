#!/bin/bash
#
TEST="Test table against CSRT"
NAME=test-0001.sh
TMPLOG=$TMP/csrt.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/csrt-0001/acpidump-0001.log csrt - | cut -c7- | grep "^csrt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/csrt-0001/csrt-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
