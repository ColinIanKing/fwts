#!/bin/bash
#
TEST="Test acpitables against WPBT"
NAME=test-0001.sh
TMPLOG=$TMP/wpbt.log.$$

$FWTS --show-tests | grep wpbt > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/wpbt-0001/acpidump-0001.log wpbt - | cut -c7- | grep "^wpbt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/wpbt-0001/wpbt-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
