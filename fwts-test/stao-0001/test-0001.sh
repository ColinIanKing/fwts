#!/bin/bash
#
TEST="Test apci table against STAO"
NAME=test-0001.sh
TMPLOG=$TMP/stao.log.$$

$FWTS --show-tests | grep STAO > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/stao-0001/acpidump-0001.log stao - | cut -c7- | grep "^stao" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/stao-0001/stao-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
