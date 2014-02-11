#!/bin/bash
#
TEST="Test --log-width=80"
NAME=test-0005.sh
TMPLOG=$TMP/klog.log.$$

$FWTS --log-format="%line %owner " --log-width=80 --klog=klog.txt klog - | grep "^[0-9]*[ ]*klog" | cut -c7- > $TMPLOG
diff $TMPLOG klog-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
