#!/bin/bash
set -e

gxx_compiler=g++
gcc_compiler=gcc

plugin_include_path=$(gcc -print-file-name=plugin)
infer_engine_path=$(ai4c-gcc --inference-engine)

model_path=../../../../models/block_correction_model.onnx

$gxx_compiler -std=c++17 -Wall -fno-rtti  \
	-I$plugin_include_path/include        \
	-fPIC -c -o block_correction_plugin.o \
	block_correction_plugin.cpp

$gxx_compiler -static-libstdc++ -shared \
    -o block_correction_plugin.so     \
    block_correction_plugin.o

$gcc_compiler test.c -O2 -o test -funroll-loops                    \
    -fplugin=./block_correction_plugin.so                            \
    -fplugin-arg-block_correction_plugin-model=$model_path         \
    -fplugin-arg-block_correction_plugin-engine=$infer_engine_path

if [ $# -eq 0  ]; then
    exit 0
fi

if [ "$1" = "clean" ]; then
    rm -f block_correction_plugin.o
    rm -f block_correction_plugin.so
    rm -f test
fi
