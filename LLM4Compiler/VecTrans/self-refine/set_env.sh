#!/bin/bash

export BISHENG_HOME=~/BiShengCompiler
export LLM_BASE_URL="https://api.siliconflow.cn/v1"
export API_KEY=""
export ENGINE="Pro/deepseek-ai/DeepSeek-V3"
export ALIVE2_HOME=~/ALIVE2
#
export PATH=$BISHENG_HOME/bin:$ALIVE2_HOME:$PATH
export LD_LIBRARY_LIB=$BISHENG_HOME/lib:$LD_LIBRARY_LIB
#
export CLANG_PATH=$BISHENG_HOME/bin/clang
export PYTHONPATH=".:../:.:src:../:../../:.:prompt-lib"
