#!/bin/bash
set -e
# export PATH=/path/to/gcc/bin:$PATH

ai4c-autotune autorun test_unroll.ini \
    -scf search_space.yaml \
    --stage-order loop \
    --time-after-convergence=10
    # --add-func-instrument=unroll_func_instrument

ai4c-autotune dump -c autotune_datadir/input.yaml \
    --database=opentuner.db/localhost.localdomain.db -o unroll

if [[ "$1" == "clean" ]]; then
    rm -rf autotune_datadir
    rm -rf opentuner.db
    rm -f opentuner.log
    rm -rf loop.*
    rm -rf unroll_*
    rm -rf test_unroll
fi
