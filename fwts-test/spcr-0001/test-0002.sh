#!/bin/bash
#
TEST="Test acpitables against invalid SPCR"
NAME=test-0001.sh
TMPLOG=$TMP/spcr.log.$$

$FWTS --show-tests | grep SPCR > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/spcr-0001/acpidump-0002.log spcr - | cut -c7- | grep "^spcr" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/spcr-0001/spcr-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
