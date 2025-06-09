#!/bin/bash
#
TEST="Test acpitables against ASPT"
NAME=test-0003.sh
TMPLOG=$TMP/aspt.log.$$

$FWTS --show-tests | grep ASPT > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/aspt-0001/acpidump-0003.log aspt - | cut -c7- | grep "^aspt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/aspt-0001/aspt-0003.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
