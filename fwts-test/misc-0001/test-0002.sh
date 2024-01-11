#!/bin/bash
#
TEST="Test acpitables against MISC"
NAME=test-0002.sh
TMPLOG=$TMP/misc.log.$$

$FWTS --show-tests | grep misc > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/misc-0001/acpidump-0002.log misc - | cut -c7- | grep "^misc" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/misc-0001/misc-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
