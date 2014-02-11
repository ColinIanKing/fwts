#!/bin/bash
#
TEST="Test --quiet option"
NAME=test-0001.sh
TMPLOG=$TMP/klog.log.$$

$FWTS --quiet -w 80 -j $FWTSTESTDIR/../data --klog=$FWTSTESTDIR/arg-quiet-0001/klog.txt klog -r /dev/null > $TMPLOG
diff $TMPLOG /dev/null >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
