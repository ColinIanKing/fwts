#!/bin/bash
#
TEST="Test acpitables against BGRT"
NAME=test-0001.sh
TMPLOG=$TMP/bgrt.log.$$

$FWTS --show-tests | grep bgrt > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/bgrt-0001/acpidump-0001.log bgrt - | cut -c7- | grep "^bgrt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/bgrt-0001/bgrt-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
