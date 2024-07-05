#!/bin/bash
#
TEST="Test acpitables against S3PT"
NAME=test-0001.sh
TMPLOG=$TMP/s3pt.log.$$

$FWTS --show-tests | grep s3pt > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/s3pt-0001/acpidump-0001.log s3pt - | cut -c7- | grep "^s3pt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/s3pt-0001/s3pt-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
