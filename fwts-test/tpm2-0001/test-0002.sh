#!/bin/bash
#
TEST="Test table against invalid TPM2"
NAME=test-0001.sh
TMPLOG=$TMP/tpm2.log.$$

$FWTS --show-tests | grep TPM2 > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/tpm2-0001/acpidump-0002.log tpm2 - | cut -c7- | grep "^tpm2" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/tpm2-0001/tpm2-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
