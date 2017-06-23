#!/bin/bash
#
TEST="Test acpitables against invalid DBGP"
NAME=test-0001.sh
TMPLOG=$TMP/dbgp.log.$$

$FWTS --show-tests | grep DBGP > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/dbgp-0001/acpidump-0002.log dbgp - | cut -c7- | grep "^dbgp" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/dbgp-0001/dbgp-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
