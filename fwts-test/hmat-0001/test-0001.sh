#!/bin/bash
#
TEST="Test acpitables against HMAT"
NAME=test-0001.sh
TMPLOG=$TMP/hmat.log.$$

$FWTS --show-tests | grep hmat > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/hmat-0001/acpidump-0001.log hmat - | cut -c7- | grep "^hmat" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/hmat-0001/hmat-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
