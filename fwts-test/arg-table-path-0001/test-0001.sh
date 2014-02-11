#!/bin/bash
#
TEST="Test --table-path to load dumped tables produced by acpixtract"
NAME=test-0001.sh
TMPLOG=$TMP/acpidump.log.$$
SORTED=$TMP/acpidump.log.sorted
HERE=$FWTSTESTDIR/arg-table-path-0001

#
# Unfortunately we can pull in the tables in different order depending
# on the way the tables are stored in the directory. Since we only care
# about output content and not necessary table order, we sort and uniq
# the output before we diff.
#
$FWTS --log-format="%line %owner " -w 80 --table-path=$HERE acpidump - | grep "^[0-9]*[ ]*acpidump" | cut -c7- | sort | uniq > $TMPLOG
sort $HERE/acpidump-0001.log > $SORTED
diff $TMPLOG $SORTED >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG $SORTED
exit $ret
