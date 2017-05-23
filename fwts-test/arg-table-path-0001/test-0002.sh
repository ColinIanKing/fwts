#!/bin/bash
#
TEST="Test -t to load dumped tables produced by acpixtract"
NAME=test-0002.sh
TMPLOG=$TMP/acpidump.log.$$
HERE=$FWTSTESTDIR/arg-table-path-0001

$FWTS --show-tests | grep acpidump > /dev/null
if [ $? -eq 1 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

machine=$(uname -m)
case $machine in
x86 | x86_32 | x86_64 | i686 )
	;;
*)
        echo SKIP: $TEST, $NAME
        exit 77
	;;
esac

#
# Unfortunately we can pull in the tables in different order depending
# on the way the tables are stored in the directory. Since we only care
# about output content and not necessary table order, we sort and uniq
# the output before we diff.
#
$FWTS --log-format="%line %owner " -w 200 -t $HERE acpidump - | grep "^[0-9]*[ ]*acpidump" | cut -c7- | LC_ALL=C sort | uniq > $TMPLOG
cat $HERE/acpidump-0001.log | grep "^[0-9]*[ ]*acpidump" | cut -c1- | LC_ALL=C sort | uniq > $TMPLOG.orig
diff $TMPLOG $TMPLOG.orig >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG $TMPLOG.orig
exit $ret
