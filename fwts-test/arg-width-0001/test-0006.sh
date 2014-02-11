#!/bin/bash
#
TEST="Test --log-width=90"
NAME=test-0006.sh
TMPLOG=$TMP/klog.log.$$

$FWTS --log-format="%line %owner " --log-width=90 --klog=klog.txt klog - | grep "^[0-9]*[ ]*klog" | cut -c7- > $TMPLOG
diff $TMPLOG klog-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
