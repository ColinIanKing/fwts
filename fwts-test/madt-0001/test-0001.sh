#!/bin/bash
#
TEST="Test against known correct ACPI APIC tables"
NAME=test-0001.sh
TMPLOG=$TMP/madt.log.$$

$FWTS --show-tests | grep MADT > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/madt-0001/acpidump-0001.log madt - | cut -c7- | grep "^madt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/madt-0001/madt-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
