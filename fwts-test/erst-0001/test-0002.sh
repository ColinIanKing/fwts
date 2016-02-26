#!/bin/bash
#
TEST="Test table against invalid ERST"
NAME=test-0002.sh
TMPLOG=$TMP/erst.log.$$

$FWTS --show-tests | grep ERST > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/erst-0001/acpidump-0002.log erst - | cut -c7- | grep "^erst" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/erst-0001/erst-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
