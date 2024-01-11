#!/bin/bash
#
TEST="Test acpitables against CCEL"
NAME=test-0001.sh
TMPLOG=$TMP/ccel.log.$$

$FWTS --show-tests | grep ccel > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/ccel-0001/acpidump-0001.log ccel - | cut -c7- | grep "^ccel" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/ccel-0001/ccel-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
