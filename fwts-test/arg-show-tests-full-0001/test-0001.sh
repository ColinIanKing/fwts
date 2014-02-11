#!/bin/bash
#
TEST="Test --show-tests-full option"
NAME=test-0001.sh
TMPLOG=$TMP/arg-show-tests-full.log.$$

stty cols 80
$FWTS --show-tests-full > $TMPLOG
diff $TMPLOG arg-show-tests-full-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
