#!/bin/bash
set -e
# export PATH=/path/to/gcc/bin:$PATH
# bash run_tuner.sh /path/to/ai4c/lib clean

rm -rf autotune_datadir
mkdir -p autotune_datadir/opp

g++ test_unroll.cc -o test_unroll -O2 -funroll-loops \
    -fplugin=$1/rtl_unroll_autotune_plugin_gcc12.so \
    -fplugin-arg-rtl_unroll_autotune_plugin_gcc12-generate=autotune_datadir/opp

ai4c-tune minimize --search-space=search_space.yaml --trials=1

g++ test_unroll.cc -o test_unroll -O2 -funroll-loops \
    -fplugin=$1/rtl_unroll_autotune_plugin_gcc12.so \
    -fplugin-arg-rtl_unroll_autotune_plugin_gcc12-autotune=autotune_datadir/config.yaml

time -p ./test_unroll 5 2>&1 1>/dev/null | grep real | awk '{print $2}'

ai4c-tune feedback 0.22

if [[ "$2" == "clean" ]]; then
    rm -rf autotune_datadir
    rm -rf opentuner.db
    rm -f opentuner.log
    rm -rf loop.*
    rm -rf unroll_*
    rm -rf test_unroll
fi
