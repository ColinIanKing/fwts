#!/bin/bash
#
TEST="Test --log-format, %line field"
NAME=test-0003.sh
TMPLOG=$TMP/klog.log.$$
TODAY=`date +%d/%m/%y`

$FWTS -w 80 --klog=klog.txt --log-format="%owner %line: " klog - | grep "^klog" | sed sx12\/10\/11x${TODAY}x > $TMPLOG
diff $TMPLOG klog-0003.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
