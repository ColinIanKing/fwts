#!/bin/bash
#
TEST="Test acpitables against invalid WSMT"
NAME=test-0001.sh
TMPLOG=$TMP/wsmt.log.$$

$FWTS --show-tests | grep wsmt > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/wsmt-0001/acpidump-0002.log wsmt - | cut -c7- | grep "^wsmt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/wsmt-0001/wsmt-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
