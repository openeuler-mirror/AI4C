#!/bin/bash
set -e
# export PATH=/path/to/gcc/bin:$PATH

ai4c-autotune autorun test_coarse_tuning.ini \
    -scf search_space.yaml \
    --stage-order function \
    --time-after-convergence=10

if [[ "$1" == "clean" ]]; then
    rm -rf autotune_datadir
    rm -rf opentuner.db
    rm -f opentuner.log
    rm -rf coarse_tuning_*
    rm -rf test_coarse_tuning
    rm -rf function.*
fi
