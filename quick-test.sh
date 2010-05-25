#!/bin/bash

LOG=results.log

echo "Results being dumped into $LOG"
sudo ./src/testsuite --stdout-summary --show-progress --results-output=$LOG
