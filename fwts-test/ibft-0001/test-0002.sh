#!/bin/bash
#
TEST="Test apci table against iBFT"
NAME=test-0002.sh
TMPLOG=$TMP/ibft.log.$$

$FWTS --show-tests | grep iBFT > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/ibft-0001/acpidump-0002.log ibft - | cut -c7- | grep "^ibft" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/ibft-0001/ibft-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
