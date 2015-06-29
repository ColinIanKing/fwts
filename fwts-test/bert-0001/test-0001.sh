#!/bin/bash
#
TEST="Test apcitables against BERT"
NAME=test-0001.sh
TMPLOG=$TMP/bert.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/bert-0001/acpidump-0001.log bert - | cut -c7- | grep "^bert" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/bert-0001/bert-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
