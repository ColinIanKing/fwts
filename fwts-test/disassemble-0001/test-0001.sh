#!/bin/bash
#
NAME=test-0001.sh

TMPDIR=$TMP/disassemble-aml
TMPLOG=$TMP/disassemble-aml.log.$$
HERE=$FWTSTESTDIR/disassemble-0001

($FWTS fwts --disassemble-aml 2>&1 | grep "option not available") > /dev/null
if [ $? -eq 0 ]; then
	echo SKIP: $TEST, $NAME
	exit 77
fi

rm -rf $TMPDIR
mkdir $TMPDIR
$FWTS -w 80 --dumpfile=$HERE/acpidump.log --disassemble-aml=$TMPDIR - > $TMPLOG

failed=0
TEST="Test --disassemble-aml output to stdout"
diff $HERE/disassemble-aml-0001.log $TMPLOG >> $FAILURE_LOG
if [ $? -eq 0 ]; then 
	echo PASSED: $TEST, $NAME
else
	echo FAILED: $TEST, $NAME
	failed=1
fi

for I in DSDT0 SSDT1 SSDT2 SSDT3 SSDT4 SSDT5
do
	TEST="Test --disassemble-aml against known $I"
	#
	#  Remove lines that contain a tmp file output name in disassembly
	#
	grep -v "DefinitionBlock" $TMPDIR/$I.dsl | grep -v "Disassembler version" | grep -v "Disassembly of"  | grep -v "Copyright" > $TMPDIR/$I.dsl.fixed.$$
	grep -v "DefinitionBlock" $HERE/$I.dsl.original | grep -v "Disassembler version" | grep -v "Disassembly of" | grep -v "Copyright" > $TMPDIR/$I.dsl.orig.fixed.$$
	
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
