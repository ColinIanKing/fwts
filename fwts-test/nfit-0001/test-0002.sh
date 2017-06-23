#!/bin/bash
#
TEST="Test acpitables against invalid NFIT"
NAME=test-0001.sh
TMPLOG=$TMP/nfit.log.$$

$FWTS --show-tests | grep nfit > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/nfit-0001/acpidump-0002.log nfit - | cut -c7- | grep "^nfit" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/nfit-0001/nfit-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
