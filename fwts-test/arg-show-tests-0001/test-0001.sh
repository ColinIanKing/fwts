#!/bin/bash
#
TEST="Test -s option"
NAME=test-0001.sh
TMPLOG=$TMP/arg-show-tests.log.$$

#
#  Non-x86 tests don't have WMI so skip this test
#
$FWTS --show-tests | grep wmi > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

cols=$(stty -a | tr ';' '\n' | grep "columns" | cut -d' ' -f3) 2> /dev/null
#
#  If we can't set the tty then we can't test
#
stty cols 80 2> /dev/null
if [ $? -eq 1 ]; then
        tset 2> /dev/null
        echo SKIP: $TEST, $NAME
        exit 77
fi
$FWTS -s > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/arg-show-tests-0001/arg-show-tests-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

stty cols 80 2> /dev/null
tset 2> /dev/null

rm $TMPLOG
exit $ret
