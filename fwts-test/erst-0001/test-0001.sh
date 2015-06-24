#!/bin/bash
#
TEST="Test table against ERST"
NAME=test-0001.sh
TMPLOG=$TMP/erst.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/erst-0001/acpidump-0001.log erst - | cut -c7- | grep "^erst" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/erst-0001/erst-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
