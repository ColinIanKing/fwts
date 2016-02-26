#!/bin/bash
#
TEST="Test table against MCHI"
NAME=test-0001.sh
TMPLOG=$TMP/mchi.log.$$

$FWTS --show-tests | grep MCHI > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/mchi-0001/acpidump-0001.log mchi - | cut -c7- | grep "^mchi" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/mchi-0001/mchi-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
