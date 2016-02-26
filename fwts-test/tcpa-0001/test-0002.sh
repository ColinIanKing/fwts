#!/bin/bash
#
TEST="Test table against invalid TCPA"
NAME=test-0001.sh
TMPLOG=$TMP/tcpa.log.$$

$FWTS --show-tests | grep TCPA > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/tcpa-0001/acpidump-0002.log tcpa - | cut -c7- | grep "^tcpa" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/tcpa-0001/tcpa-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
