#!/bin/bash
#
TEST="Test apcitables against broken ACPI RSDP"
NAME=test-0001.sh
TMPLOG=$TMP/rsdp.log.$$

machine=$(uname -m)
case $machine in
x86 | x86_32 | x86_64 | i686 )
	;;
*)
        echo SKIP: $TEST, $NAME
        exit 77
	;;
esac

$FWTS --log-format="%line %owner " -w 80 --dumpfile=$FWTSTESTDIR/rsdp-0001/acpidump-0001.log rsdp - | cut -c7- | grep "^rsdp" > $TMPLOG
diff $TMPLOG $FWTSTESTDIR/rsdp-0001/rsdp.log >> $FAILURE_LOG
ret=$?
if [ $ret -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
fi

rm $TMPLOG
exit $ret
