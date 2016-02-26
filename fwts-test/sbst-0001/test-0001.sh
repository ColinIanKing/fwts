#!/bin/bash
#
TEST="Test apcitables against SBST"
NAME=test-0001.sh
TMPLOG=$TMP/sbst.log.$$

$FWTS --show-tests | grep SBST > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/sbst-0001/acpidump-0001.log sbst - | cut -c7- | grep "^sbst" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/sbst-0001/sbst-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
