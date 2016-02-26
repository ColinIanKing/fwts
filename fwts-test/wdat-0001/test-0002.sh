#!/bin/bash
#
TEST="Test apci against invalid WDAT"
NAME=test-0002.sh
TMPLOG=$TMP/wdat.log.$$

$FWTS --show-tests | grep WDAT > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/wdat-0001/acpidump-0002.log wdat - | cut -c7- | grep "^wdat" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/wdat-0001/wdat-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
