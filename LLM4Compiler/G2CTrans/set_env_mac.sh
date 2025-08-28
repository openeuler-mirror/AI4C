#!/bin/bash

export OPENAI_API_BASE="https://api.siliconflow.cn/v1"
export OPENAI_API_KEY=""   # to be filled
export ENGINE="Pro/deepseek-ai/DeepSeek-V3"

# LLVM & Clang configuration
export SDKROOT=$(xcrun --show-sdk-path)

export LLVM_HOME=~/install_llvm20
export PATH=$LLVM_HOME/bin:$PATH
export DYLD_LIBRARY_PATH=$LLVM_HOME/lib:$DYLD_LIBRARY_PATH
export LIBCLANG=$LLVM_HOME/lib

export CC=$LLVM_HOME/bin/clang
export CXX=$LLVM_HOME/bin/clang++

export CLANG_PATH=$CXX

export Z3_HOME=~/install_new
export DYLD_LIBRARY_PATH=$Z3_HOME/lib:$DYLD_LIBRARY_PATH

export ALIVE2_HOME=~/build
export PATH=$ALIVE2_HOME:$PATH

