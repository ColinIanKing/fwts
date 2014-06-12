#!/bin/bash
#
TEST="Test -D option"
NAME=test-0002.sh
TMPLOG=$TMP/progress.log.$$
TMPLOG_ORIG=$TMP/progress-0001.log.$$

#
#  Normalise % progress and seconds to run so it works OK
#  with different progress roundings and test durations on
#  different architectures.
#
fixup()
{
	sed s'X[0-9]*\.[0-9]*%X00.00%X' | sed s'X[0-9]* secondsX0 secondsX'
}

$FWTS -w 80 -D -j $FWTSTESTDIR/../data --klog=$FWTSTESTDIR/arg-show-progress-dialog-0001/klog.txt oops klog | fixup > $TMPLOG
fixup < $FWTSTESTDIR/arg-show-progress-dialog-0001/progress-0001.log > $TMPLOG_ORIG
diff $TMPLOG $TMPLOG_ORIG >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG $TMPLOG_ORIG
exit $ret
