#!/bin/bash

LOOP_COUNT=128
TEMPFILE="/tmp/timecntr"
TEMPFILETOTALS="/tmp/totals"


objc_run_test() {
	TEST_NAME="$1"
	echo "${TEST_NAME}:"
	./${TEST_NAME}
}

for TEST in ./*-test; do
	objc_run_test $TEST 0
	echo "======================"
done

