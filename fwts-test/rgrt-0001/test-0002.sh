#!/bin/bash
#
TEST="Test acpitables against RGRT"
NAME=test-0001.sh
TMPLOG=$TMP/rgrt.log.$$

$FWTS --show-tests | grep rgrt > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/rgrt-0001/acpidump-0002.log rgrt - | cut -c7- | grep "^rgrt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/rgrt-0001/rgrt-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
