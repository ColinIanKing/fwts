#!/bin/bash
#
TEST="Test -h option"
NAME=test-0001.sh
TMPLOG=$TMP/help.log.$$
HERE=`pwd`

#
#  Non-x86 tests don't have WMI so skip this test
#
$FWTS --show-tests | grep wmi > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

stty cols 80
$FWTS -h | grep -v "Show version" | grep -v "Usage" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/arg-help-0001/arg-help-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
