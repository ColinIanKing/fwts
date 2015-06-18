#!/bin/bash
#
TEST="Test apcitables against broken ACPI XSDT table"
NAME=test-0001.sh
TMPLOG=$TMP/xsdt.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/xsdt-0001/acpidump-0001.log xsdt - | cut -c7- | grep "^xsdt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/xsdt-0001/xsdt.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
