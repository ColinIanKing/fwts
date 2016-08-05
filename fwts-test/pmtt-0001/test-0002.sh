#!/bin/bash
#
TEST="Test apcitables against invalid PMTT"
NAME=test-0001.sh
TMPLOG=$TMP/pmtt.log.$$

$FWTS --show-tests | grep pmtt > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/pmtt-0001/acpidump-0002.log pmtt - | cut -c7- | grep "^pmtt" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/pmtt-0001/pmtt-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
