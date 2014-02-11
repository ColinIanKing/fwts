#!/bin/bash
#
TEST="Test -D option"
NAME=test-0001.sh
TMPLOG=$TMP/progress.log.$$

$FWTS -w 80 -D --klog=klog.txt oops klog version > $TMPLOG
diff $TMPLOG progress-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
