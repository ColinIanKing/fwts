#!/bin/bash
#
TEST="Test apci table against invalid IVRS"
NAME=test-0001.sh
TMPLOG=$TMP/ivrs.log.$$

$FWTS --show-tests | grep IVRS > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/ivrs-0001/acpidump-0002.log ivrs - | cut -c7- | grep "^ivrs" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/ivrs-0001/ivrs-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
