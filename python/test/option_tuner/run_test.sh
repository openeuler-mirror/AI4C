#!/bin/bash
set -e
# export PATH=/path/to/gcc/bin:$PATH
# export LD_LIBRARY_PATH=/path/to/gcc/lib64:$LD_LIBRARY_PATH

if [ $# -eq 0 ]; then
    echo "No arguments were provided"
    exit 0
fi

parent_dir=$1
# config: current compiler configuration file
config=$(cat ${parent_dir}/tuning/config.txt)
# performance_file: current performance data file
performance_file="${parent_dir}/tuning/performance.txt"

# File path change start from here.
# If there is more than one measurement, remember to change the logic of
# function `get_performance` and `calc_performance` in `measure.py` as well as
# the measurement weights in `config_measure.yaml`.

### environment variable ###
measure_raw_file="time.txt"

### compilation script ###
compiler=g++
compile_command="${compiler} test.cc -O2 -o test_opt_tuner"
eval "${compile_command} ${config}"

### execution script ###
run_command="time -p -o ${measure_raw_file} ./test_opt_tuner 3"
eval "${run_command}"

info_collect_command="grep real ${measure_raw_file} | awk '{printf \"1 1 %s\", \$2}' > ${performance_file}"
eval "${info_collect_command}"
