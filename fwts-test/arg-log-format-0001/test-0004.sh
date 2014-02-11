#!/bin/bash
#
TEST="Test --log-format, %date %level field operators"
NAME=test-0004.sh
TMPLOG=$TMP/klog.log.$$
TODAY=`date +%d/%m/%y`
TMPLOG_ORIG=$TMP/klog-0004.log.$$
TODAY=`date +%d/%m/%y`

#
#  Need to adjust reference log to today's date
#
$FWTS -w 80 -j $FWTSTESTDIR/../data --klog=$FWTSTESTDIR/arg-log-format-0001/klog.txt --log-format="%owner %date %%level: " klog - | grep "^klog" > $TMPLOG
sed sx10\/10\/11x${TODAY}x < $FWTSTESTDIR/arg-log-format-0001/klog-0004.log > $TMPLOG_ORIG
diff $TMPLOG $TMPLOG_ORIG >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG $TMPLOG_ORIG
exit $ret
