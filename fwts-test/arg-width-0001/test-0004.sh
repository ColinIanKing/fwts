#!/bin/bash
#
TEST="Test -w 1000"
NAME=test-0004.sh
TMPLOG=$TMP/klog.log.$$

$FWTS --log-format="%line %owner " -w 1000 -j $FWTSTESTDIR/../data  --klog=$FWTSTESTDIR/arg-width-0001/klog.txt klog - | grep "^[0-9]*[ ]*klog" | cut -c7- > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/arg-width-0001/klog-0004.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
