#!/bin/bash
#
TEST="Test syntaxcheck against known correct ACPI tables"
NAME=test-0001.sh
TMPLOG=$TMP/syntaxcheck.log.$$

$FWTS --log-format="%line %owner " -w 80 --dumpfile=acpidump-0001.log syntaxcheck - | grep "^[0-9]*[ ]*syntaxcheck" | cut -c7- > $TMPLOG
diff $TMPLOG syntaxcheck-0001.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
