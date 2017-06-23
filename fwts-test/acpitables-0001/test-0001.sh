#!/bin/bash
#
TEST="Test acpitables against known correct ACPI tables"
NAME=test-0001.sh
TMPLOG=$TMP/acpitables.log.$$

$FWTS --show-tests | grep acpitables > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/acpitables-0001/acpidump-0001.log acpitables - | grep "^[0-9]*[ ]*acpitables" | cut -c7- > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/acpitables-0001/acpitables-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
