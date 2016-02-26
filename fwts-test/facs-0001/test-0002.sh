#!/bin/bash
#
TEST="Test table against invalid FACS"
NAME=test-0001.sh
TMPLOG=$TMP/facs.log.$$

$FWTS --show-tests | grep FACS > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/facs-0001/acpidump-0002.log facs - | cut -c7- | grep "^facs" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/facs-0001/facs-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
