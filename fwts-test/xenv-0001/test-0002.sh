#!/bin/bash
#
TEST="Test table against invalid XENV"
NAME=test-0001.sh
TMPLOG=$TMP/xenv.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/xenv-0001/acpidump-0002.log xenv - | cut -c7- | grep "^xenv" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/xenv-0001/xenv-0002.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
