#!/bin/bash
#
TEST="Test table against UEFI"
NAME=test-0001.sh
TMPLOG=$TMP/uefi.log.$$

$FWTS --show-tests | grep "UEFI Data" > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/uefi-0001/acpidump-0001.log uefi - | cut -c7- | grep "^uefi" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/uefi-0001/uefi-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
