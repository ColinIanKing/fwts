#!/bin/bash
fwts=../src/fwts

run_arg_tests()
{
	echo Test: $1
	sudo valgrind --leak-check=full --show-reachable=yes -v $fwts $1 2>&1 | grep "ERROR SUMMARY:"
}

run_tests()
{
	tests=`fwts --show-tests | grep -v "Available tests" | awk '{ print $1'}`
	for test in $tests
	do
		echo Test: $test
		sudo valgrind --leak-check=full --show-reachable=yes -v $fwts $test --no-s3 --no-s4 2> errors.txt
		grep "ERROR SUMMARY:" errors.txt
		rm errors.txt
	done
}
run_arg_tests "--dump"
run_arg_tests "--version"
run_arg_tests "--debug-output=stdout osilinux"
run_arg_tests "--force_clean osilinux"
run_arg_tests "--force_clean --results-output=/dev/null osilinux"
run_arg_tests "--force_clean --show-progress osilinux"
run_arg_tests "--force_clean --fwts-debug -progress osilinux"
run_arg_tests "--help"

run_tests

