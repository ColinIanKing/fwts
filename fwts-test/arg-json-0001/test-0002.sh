#!/bin/bash
#
TEST="Test -j option"
NAME=test-0002.sh
TMPLOG=$TMP/klog.log.$$
HERE=$FWTSTESTDIR/../data

$FWTS --log-format="%line %owner " -w 80 -j $HERE --klog=$FWTSTESTDIR/arg-json-0001/klog.txt klog - | grep "^[0-9]*[ ]*klog" | cut -c7- > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/arg-json-0001/klog-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
