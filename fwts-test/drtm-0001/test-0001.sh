#!/bin/bash
#
TEST="Test apcitables against DRTM"
NAME=test-0001.sh
TMPLOG=$TMP/drtm.log.$$

$FWTS --show-tests | grep drtm > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/drtm-0001/acpidump-0001.log drtm - | cut -c7- | grep "^drtm" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/drtm-0001/drtm-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
