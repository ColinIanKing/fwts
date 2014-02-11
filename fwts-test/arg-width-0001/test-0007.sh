#!/bin/bash
#
TEST="Test --log-width=100"
NAME=test-0007.sh
TMPLOG=$TMP/klog.log.$$

$FWTS --log-format="%line %owner " --log-width=100 -j $FWTSTESTDIR/../data  --klog=$FWTSTESTDIR/arg-width-0001/klog.txt klog - | grep "^[0-9]*[ ]*klog" | cut -c7- > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/arg-width-0001/klog-0003.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
