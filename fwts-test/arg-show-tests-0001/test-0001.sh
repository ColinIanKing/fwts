#!/bin/bash
#
TEST="Test -s option"
NAME=test-0001.sh
TMPLOG=$TMP/arg-show-tests.log.$$

stty cols 80
$FWTS -s > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/arg-show-tests-0001/arg-show-tests-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
