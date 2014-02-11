#!/bin/bash
#
TEST="Test -q option"
NAME=test-0001.sh
TMPLOG=$TMP/klog.log.$$

$FWTS -q -w 80 --klog=klog.txt klog -r /dev/null > $TMPLOG
diff $TMPLOG /dev/null >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
