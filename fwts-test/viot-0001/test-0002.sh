#!/bin/bash
#
TEST="Test acpitables against VIOT"
NAME=test-0002.sh
TMPLOG=$TMP/viot.log.$$

$FWTS --show-tests | grep viot > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/viot-0001/acpidump-0002.log viot - | cut -c7- | grep "^viot" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/viot-0001/viot-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
