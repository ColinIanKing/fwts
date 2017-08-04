#!/bin/bash
#
TEST="Test acpitables against invalid SDEI"
NAME=test-0001.sh
TMPLOG=$TMP/sdei.log.$$

$FWTS --show-tests | grep sdei > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/sdei-0001/acpidump-0002.log sdei - | cut -c7- | grep "^sdei" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/sdei-0001/sdei-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
