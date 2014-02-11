#!/bin/bash
#
TEST="Test --log-format, %owner field"
NAME=test-0002.sh
TMPLOG=$TMP/klog.log.$$

$FWTS -w 80 --klog=klog.txt --log-format="%owner: " klog - | grep "^klog" > $TMPLOG
diff $TMPLOG klog-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
