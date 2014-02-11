#!/bin/bash
#
TEST="Test --log-fields"
NAME=test-0001.sh
TMPLOG=$TMP/logfields.log.$$

$FWTS --log-fields > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/arg-log-fields-0001/logfields-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
