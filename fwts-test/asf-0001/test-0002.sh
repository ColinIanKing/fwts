#!/bin/bash
#
TEST="Test apci against invalid ASF!"
NAME=test-0002.sh
TMPLOG=$TMP/asf.log.$$

$FWTS --show-tests | grep ASF > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/asf-0001/acpidump-0002.log asf - | cut -c7- | grep "^asf" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/asf-0001/asf-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
