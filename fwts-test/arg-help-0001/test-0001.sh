#!/bin/bash
#
TEST="Test -h option"
NAME=test-0001.sh
TMPLOG=$TMP/help.log.$$
HERE=`pwd`

stty cols 80
$FWTS -h | grep -v "Show version" | grep -v "Usage" > $TMPLOG
diff $TMPLOG arg-help-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
