#!/bin/bash
#
TEST="Test table against MPST"
NAME=test-0001.sh
TMPLOG=$TMP/mpst.log.$$

$FWTS --show-tests | grep MPST > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/mpst-0001/acpidump-0001.log mpst - | cut -c7- | grep "^mpst" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/mpst-0001/mpst-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
