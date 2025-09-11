#!/bin/bash
#
TEST="Test acpitables against MSCT"
NAME=test-0002.sh
TMPLOG=$TMP/msct.log.$$

$FWTS --show-tests | grep msct > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/msct-0001/acpidump-0002.log msct - | cut -c7- | grep "^msct" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/msct-0001/msct-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
