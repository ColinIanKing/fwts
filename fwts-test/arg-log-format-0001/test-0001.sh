#!/bin/bash
#
TEST="Test --log-format, all field operators"
NAME=test-0001.sh
TMPLOG=$TMP/klog.log.$$
TMPLOG_ORIG=$TMP/klog-0001.log.$$
TODAY=`date +%d/%m/%y`

$FWTS -w 80 -j $FWTSTESTDIR/../data --klog=$FWTSTESTDIR/arg-log-format-0001/klog.txt --log-format="%owner (%line) <%date> %field %level: " klog - | grep "^klog" | sed "sx([0-9][0-9][0-9][0-9][0-9])x(XXXXX)x" > $TMPLOG
#
#  Need to adjust reference log to today's date
#
cat $FWTSTESTDIR/arg-log-format-0001/klog-0001.log | sed "sx<[0-9][0-9]\/[0-9][0-9]\/[0-9][0-9]>x<${TODAY}>x" | sed "sx([0-9][0-9][0-9][0-9][0-9])x(XXXXX)x" > $TMPLOG_ORIG
diff $TMPLOG $TMPLOG_ORIG >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG $TMPLOG_ORIG
exit $ret
