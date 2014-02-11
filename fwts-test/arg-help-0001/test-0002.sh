#!/bin/bash
#
TEST="Test --help option"
NAME=test-0002.sh
TMPLOG=$TMP/help.log.$$
HERE=`pwd`

stty cols 80

$FWTS --help | grep -v "Show version" | grep -v "Usage" > $TMPLOG
diff $TMPLOG fwts-test/arg-help-0001/arg-help-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
