#!/bin/bash
#
TEST="Test --show-tests option"
NAME=test-0002.sh
TMPLOG=$TMP/arg-show-tests.log.$$

stty cols 80
$FWTS -s > $TMPLOG
diff $TMPLOG arg-show-tests-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
