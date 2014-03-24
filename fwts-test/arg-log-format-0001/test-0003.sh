#!/bin/bash
#
TEST="Test --log-format, %line field"
NAME=test-0003.sh
TMPLOG=$TMP/klog.log.$$
TMPLOG_ORIG=$TMP/klog-0003.log.$$
TODAY=`date +%d/%m/%y`

$FWTS -w 80 -j $FWTSTESTDIR/../data --klog=$FWTSTESTDIR/arg-log-format-0001/klog.txt --log-format="%owner %line: " klog - | grep "^klog" | sed sx12\/10\/11x${TODAY}x | sed "sx[0-9][0-9][0-9][0-9][0-9]:xXXXXX:x" > $TMPLOG
sed "sx[0-9][0-9][0-9][0-9][0-9]:xXXXXX:x" < $FWTSTESTDIR/arg-log-format-0001/klog-0003.log > $TMPLOG_ORIG
diff $TMPLOG $TMPLOG_ORIG >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
	cat $FAILURE_LOG
fi

rm $TMPLOG $TMPLOG_ORIG
exit $ret
