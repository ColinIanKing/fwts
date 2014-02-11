#!/bin/bash
#
NAME=test-0001.sh

TMPDIR=$TMP/disassemble-aml
TMPLOG=$TMP/disassemble-aml.log.$$
HERE=`pwd`

mkdir $TMPDIR
$FWTS -w 80 --dumpfile=$HERE/acpidump.log --disassemble-aml=$TMPDIR - > $TMPLOG

failed=0
TEST="Test --disassemble-aml output to stdout"
diff disassemble-aml-0001.log $TMPLOG >> $FAILURE_LOG
if [ $? -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
	failed=1
fi

for I in DSDT SSDT0 SSDT1 SSDT2 SSDT3 SSDT4
do
	TEST="Test --disassemble-aml against known $I"
	#
	#  Remove lines that contain a tmp file output name in disassembly
	#
	grep -v "/tmp/fwts_iasl" $TMPDIR/$I.dsl | grep -v "AML Disassembler version"  > $TMPDIR/$I.dsl.fixed.$$
	grep -v "/tmp/fwts_iasl" $I.dsl.orig | grep -v "AML Disassembler version" > $TMPDIR/$I.dsl.orig.fixed.$$
	
	diff $TMPDIR/$I.dsl.fixed.$$ $TMPDIR/$I.dsl.orig.fixed.$$
	if [ $? -eq 0 ]; then 
		echo PASSED: $TEST, $NAME
	else
		echo FAILED: $TEST, $NAME
		failed=1
	fi

	rm $TMPDIR/$I.dsl.fixed.$$ $TMPDIR/$I.dsl.orig.fixed.$$
done

rm -rf $TMPDIR $TMPLOG
exit $failed
