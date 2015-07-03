#!/bin/bash
#
TEST="Test table against MSDM"
NAME=test-0001.sh
TMPLOG=$TMP/msdm.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/msdm-0001/acpidump-0001.log msdm - | cut -c7- | grep "^msdm" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/msdm-0001/msdm-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
