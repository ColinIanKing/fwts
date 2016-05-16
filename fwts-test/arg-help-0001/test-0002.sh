#!/bin/bash
#
TEST="Test --help option"
NAME=test-0002.sh
TMPLOG=$TMP/help.log.$$
HERE=`pwd`

#
#  Non-x86 tests don't have WMI so skip this test
#
$FWTS --show-tests | grep WMI > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

cols=$(stty -a | tr ';' '\n' | grep "columns" | cut -d' ' -f3) 2> /dev/null
#
#  If we can't set the tty then we can't test
#
stty cols 50 2> /dev/null
if [ $? -eq 1 ]; then
	tset 2> /dev/null
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --help | grep -v "Show version" | grep -v "Usage" | sed s/\([Vv][0-9][0-9]\.[0-9][0-9]\.[0-9][0-9]\)/\(Vxx\.xx\.xx\)/  > $TMPLOG
sed s/\([Vv][0-9][0-9]\.[0-9][0-9]\.[0-9][0-9]\)/\(Vxx\.xx\.xx\)/ < $FWTSTESTDIR/arg-help-0001/arg-help-0002.log > $TMP/help.log.$$.orig
diff $TMPLOG $TMP/help.log.$$.orig >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi
if [ $cols -ne 0 ]; then
	stty cols $cols 2> /dev/null
fi
tset 2> /dev/null

rm $TMPLOG ${TMPLOG}.orig
exit $ret
