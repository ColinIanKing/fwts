#!/bin/bash
#
TEST="Test klog against known common failure patterns (was the dmesg_common test)"
NAME=test-0002.sh
TMPLOG=$TMP/klog.log.$$

$FWTS --log-format="%line %owner " -w 80 -j $FWTSTESTDIR/../data --klog=$FWTSTESTDIR/klog-0002/klog.txt klog - | grep summary | sed 's/line: [0-9]*//' | cut -c7-  > $TMPLOG
grep -v "log line:" $FWTSTESTDIR/klog-0002/klog-0002.log | diff $TMPLOG - >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
