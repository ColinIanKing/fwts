#!/bin/bash
#
TEST="Test apcitables against incorrect ACPI APIC tables"
NAME=test-0002.sh
TMPLOG=$TMP/acpitables.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=acpidump-0001.log acpitables - | grep "^[0-9]*[ ]*acpitables"| cut -c7- > $TMPLOG
diff $TMPLOG acpitables-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
