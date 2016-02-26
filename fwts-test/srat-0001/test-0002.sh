#!/bin/bash
#
TEST="Test apcitables against invalid SRAT"
NAME=test-0001.sh
TMPLOG=$TMP/srat.log.$$

$FWTS --show-tests | grep SRAT > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/srat-0001/acpidump-0002.log srat - | cut -c7- | grep "^srat" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/srat-0001/srat-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
