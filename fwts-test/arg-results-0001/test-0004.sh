#!/bin/bash
#
TEST="Test -r stdout option"
NAME=test-0004.sh
TMPLOG=$TMP/results_$$.log

($FWTS --log-format="%line %owner " -w 80 --klog=klog.txt klog -r stdout | grep "^[0-9]*[ ]*klog" | cut -c7- > $TMPLOG ) >& /dev/null
diff $TMPLOG results.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
