#!/bin/bash
#
TEST="Test acpitables against invalid BOOT"
NAME=test-0001.sh
TMPLOG=$TMP/boot.log.$$

$FWTS --show-tests | grep BOOT > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/boot-0001/acpidump-0002.log boot - | cut -c7- | grep "^boot" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/boot-0001/boot-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
