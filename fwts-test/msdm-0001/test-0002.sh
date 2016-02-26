#!/bin/bash
#
TEST="Test table against invalid MSDM"
NAME=test-0001.sh
TMPLOG=$TMP/msdm.log.$$

$FWTS --show-tests | grep MSDM > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/msdm-0001/acpidump-0002.log msdm - | cut -c7- | grep "^msdm" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/msdm-0001/msdm-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
