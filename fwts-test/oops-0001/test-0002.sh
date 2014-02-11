#!/bin/bash
#
TEST="Test oops summary table against known failure patterns"
NAME=test-0002.sh
TMPLOG=$TMP/oops.log.$$

$FWTS --log-format="%line %owner " -w 400 --klog=$FWTSTESTDIR/oops-0001/oops.txt oops - | grep summary | sed 's/line: [0-9]*//' | cut -c7- > $TMPLOG
grep -v "log line:" $FWTSTESTDIR/oops-0001/oops-0002.log | diff $TMPLOG - >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
