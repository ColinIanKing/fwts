#!/bin/bash
#
TEST="Test klog against known failure rate"
NAME=test-0003.sh
TMPLOG=$TMP/klog.log.$$

$FWTS --log-format="%line %owner " -w 80 -j $FWTSTESTDIR/../data --klog=$FWTSTESTDIR/klog-0001/klog.txt klog - | grep "unique errors in kernel log" | cut -c7- > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/klog-0001/klog-0003.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
