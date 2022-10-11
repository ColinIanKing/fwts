#!/bin/bash
#
TEST="Test acpitables against CEDT"
NAME=test-0001.sh
TMPLOG=$TMP/cedt.log.$$

$FWTS --show-tests | grep cedt > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/cedt-0001/acpidump-0002.log cedt - | cut -c7- | grep "^cedt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/cedt-0001/cedt-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
