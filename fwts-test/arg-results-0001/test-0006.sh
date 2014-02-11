#!/bin/bash
#
TEST="Test -r option (append)"
NAME=test-0006.sh
TMPLOG=$TMP/results_$$.log

$FWTS --log-format="%line %owner " -w 80 --klog=klog.txt klog -r $TMPLOG >& /dev/null
$FWTS --log-format="%line %owner " -w 80 --klog=klog.txt klog -r $TMPLOG >& /dev/null
grep "^[0-9]*[ ]*klog" $TMPLOG | cut -c7- > $TMPLOG.filtered
diff $TMPLOG.filtered results-0005.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG $TMPLOG.filtered
exit $ret
