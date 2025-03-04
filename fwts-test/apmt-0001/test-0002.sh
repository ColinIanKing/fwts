#!/bin/bash
#
TEST="Test acpitables against APMT"
NAME=test-0002.sh
TMPLOG=$TMP/apmt.log.$$

$FWTS --show-tests | grep apmt > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/apmt-0001/acpidump-0002.log apmt - | cut -c7- | grep "^apmt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/apmt-0001/apmt-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
