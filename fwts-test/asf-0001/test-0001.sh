#!/bin/bash
#
TEST="Test apci table against ASF!"
NAME=test-0001.sh
TMPLOG=$TMP/asf.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/asf-0001/acpidump-0001.log asf - | cut -c7- | grep "^asf" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/asf-0001/asf-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
