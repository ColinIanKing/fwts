#!/bin/bash
#
TEST="Test oops against known failure rate"
NAME=test-0003.sh
TMPLOG=$TMP/oops.log.$$

$FWTS --log-format="%line %owner " -w 80 --klog=oops.txt oops - | grep "oopses" | grep -v "summary" | cut -c7- > $TMPLOG
diff $TMPLOG oops-0003.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
