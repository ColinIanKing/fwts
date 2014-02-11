#!/bin/bash
#
TEST="Test wmi against known correct ACPI tables"
NAME=test-0001.sh
TMPLOG=$TMP/wmi.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/wmi-0001/acpidump-0001.log wmi - | grep "^[0-9]*[ ]*wmi" | cut -c7- > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/wmi-0001/wmi-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
