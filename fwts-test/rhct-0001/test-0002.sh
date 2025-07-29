#!/bin/bash
#
TEST="Test acpitables against RHCT"
NAME=test-0002.sh
TMPLOG=$TMP/rhct.log.$$

$FWTS --show-tests | grep rhct > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/rhct-0001/acpidump-0002.log rhct - | cut -c7- | grep "^rhct" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/rhct-0001/rhct-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
