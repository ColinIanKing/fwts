#!/bin/bash
#
TEST="Test --show-tests-full option"
NAME=test-0001.sh
TMPLOG=$TMP/arg-show-tests-full.log.$$

#
#  Non-x86 tests don't have WMI so skip this test
#
$FWTS --show-tests | grep wmi > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

stty cols 80
$FWTS --show-tests-full > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/arg-show-tests-full-0001/arg-show-tests-full-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
