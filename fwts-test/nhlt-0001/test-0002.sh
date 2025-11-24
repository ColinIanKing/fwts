#!/bin/bash
#
TEST="Test acpitables against NHLT"
NAME=test-0002.sh
TMPLOG=$TMP/nhlt.log.$$

$FWTS --show-tests | grep nhlt > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/nhlt-0001/acpidump-0002.log nhlt - | cut -c7- | grep "^nhlt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/nhlt-0001/nhlt-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
