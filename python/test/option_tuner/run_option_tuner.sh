#!/bin/bash
set -e
ai4c-option-tune --test_limit 3 -r run_test.sh

### clean intermediate files ###
if [ "$1" = "clean" ]; then
    rm -f test_opt_tuner
    rm -f test.txt
    rm -f time.txt
    rm -rf tuning
    rm -rf result
    exit 0
fi
