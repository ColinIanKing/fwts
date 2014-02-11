#!/bin/bash
#
TEST="Test klog against known failure rate"
NAME=test-0003.sh
TMPLOG=$TMP/klog.log.$$

$FWTS --log-format="%line %owner " -w 80 --klog=klog.txt klog - | grep "unique errors in kernel log" | cut -c7- > $TMPLOG
diff $TMPLOG klog-0003.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
