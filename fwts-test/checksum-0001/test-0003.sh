#!/bin/bash
#
TEST="Test checksum against incorrect ACPI DSDT table"
NAME=test-0003.sh
TMPLOG=$TMP/checksum.log.$$

$FWTS --show-tests | grep checksum > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 120 --dumpfile=$FWTSTESTDIR/checksum-0001/acpidump-0003.log checksum - | grep "^[0-9]*[ ]*checksum" | grep -v RSDP | cut -c7- > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/checksum-0001/checksum-0003.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
