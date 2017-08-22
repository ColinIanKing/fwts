#!/bin/bash
#
TEST="Test acpitables against invalid RASF"
NAME=test-0001.sh
TMPLOG=$TMP/rasf.log.$$

$FWTS --show-tests | grep rasf > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/rasf-0001/acpidump-0002.log rasf - | cut -c7- | grep "^rasf" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/rasf-0001/rasf-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
