#!/bin/bash
#
TEST="Test wmi against known correct ACPI tables"
NAME=test-0002.sh
TMPLOG=$TMP/wmi.log.$$

$FWTS --show-tests | grep wmi > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/wmi-0001/acpidump-0002.log wmi - | grep "^[0-9]*[ ]*wmi" | cut -c7- > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/wmi-0001/wmi-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
