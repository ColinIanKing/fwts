#!/bin/bash
#
TEST="Test --results-no-separators option"
NAME=test-0001.sh
TMPLOG=$TMP/klog.log.$$

$FWTS --log-format="%line %owner " -w 80 -j $FWTSTESTDIR/../data --results-no-separators --klog=$FWTSTESTDIR/arg-results-no-separators-0001/klog.txt klog - | grep "^[0-9]*[ ]*klog" | cut -c7- > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/arg-results-no-separators-0001/klog-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
