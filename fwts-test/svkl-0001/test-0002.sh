#!/bin/bash
#
TEST="Test acpitables against SVKL"
NAME=test-0001.sh
TMPLOG=$TMP/svkl.log.$$

$FWTS --show-tests | grep svkl > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/svkl-0001/acpidump-0002.log svkl - | cut -c7- | grep "^svkl" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/svkl-0001/svkl-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
