#!/bin/bash
#
TEST="Test acpitables against PHAT"
NAME=test-0001.sh
TMPLOG=$TMP/phat.log.$$

$FWTS --show-tests | grep phat > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/phat-0001/acpidump-0001.log phat - | cut -c7- | grep "^phat" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/phat-0001/phat-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
