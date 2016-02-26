#!/bin/bash
#
TEST="Test apcitables against broken ACPI RSDT table"
NAME=test-0001.sh
TMPLOG=$TMP/rsdt.log.$$

$FWTS --show-tests | grep RSDT > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/rsdt-0001/acpidump-0001.log rsdt - | cut -c7- | grep "^rsdt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/rsdt-0001/rsdt-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
