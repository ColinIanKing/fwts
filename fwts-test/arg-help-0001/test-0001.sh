#!/bin/bash
#
TEST="Test -h option"
NAME=test-0001.sh
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
$FWTS -h | grep -v "Show version" | grep -v "Usage" | grep -v "Copyright" | sed s/\([Vv][0-9][0-9]\.[0-9][0-9]\.[0-9][0-9]\)/\(Vxx\.xx\.xx\)/ > $TMPLOG
grep -v "Copyright" $FWTSTESTDIR/arg-help-0001/arg-help-0001.log | sed s/\([Vv][0-9][0-9]\.[0-9][0-9]\.[0-9][0-9]\)/\(Vxx\.xx\.xx\)/  > $TMP/help.log.$$.orig
diff $TMPLOG $TMP/help.log.$$.orig >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi
stty cols 80 2> /dev/null
tset 2> /dev/null

rm $TMPLOG ${TMPLOG}.orig
exit $ret
