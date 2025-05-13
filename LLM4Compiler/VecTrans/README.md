# LLM4Compiler - VecTrans: Iterative Refinement LLM Optimization Framework for Auto-Vectorization

The developed iterative LLM Compiler is based on [self-refine](https://selfrefine.info/). Three kinds of feedback are implemented: 
(1) messages from LLVM Compiler like the option: "−Rpass−analysis=loop−vectorize" or "-debug-only=loop-vectorize";
(2) semantic correctness information from automatic testing-based method or formal methods;
(3) LLM itself(self-feedback); 

and in the future, we will add new feedback from
(4) performance from the runtime phases. -- TODO

The aimed compilation optimization including vectorization, bug-fixing and so on. The current verification is mainly for the vectorization (LLM Vectorizer) on ARM Neon/SVE intrinsics.

For more introduction for VecTrans framework, please refer to: ```Zheng Z, Cheng L, Li L, et al. VecTrans: LLM Transformation Framework for Better Auto-vectorization on High-performance CPU[J]. arXiv preprint arXiv:2503.19449, 2025.```

To use the iterative framework - VecTrans, follow the steps:

### (1) dependencies: ```anthropic```, ```openai```
```bash
pip install prompt-lib/
pip install anthropic
pip install openai
pip install --upgrade openai
```

### (2) install the formal verification tool - alive2
The full installation tutorial is attached in the end appendix. After installing the alive2, assume the installation path is: ```ALIVE2_HOME```. Run the following command to check the equivalence of the target llvm IR with source code llvm IR.
```
alive-tv source.ll target.ll
```
The following output could be printed: 
```
... ...
Transformation seems to be correct!

Summary:
  1 correct transformations
  0 incorrect transformations
  0 failed-to-prove transformations
  0 Alive2 errors
```

### (3) env setup: ```set_env.sh```
```bash
source set_env.sh
```
It is worth noting that the ```API_KEY```, ```LLM_BASE_URL```, ```LLVM_HOME```, ```ALIVE2_HOME``` and ```ENGINE``` need to be updated in the ```set_env.sh```. The default LLM model is ```deepseek v3``` from siliconflow.
```bash
#!/bin/bash

export LLVM_HOME=~/LLVMCompiler
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
```

### (4) run the iterative inference
> 
The function entrance is ```./src/vectorizer/run_split.py```. Run the a command in the self-refine directory.
```
python src/vectorizer/run_split.py
```


## Appendix A: The installation of alive2.
### the dependency requirements.
- cmake https://cmake.org/files/
- gcc (>=10.3.1)
- llvm/clang (20.1.3)
- Z3
- re2c

### (1) LLVM
```
git clone https://github.com/llvm/llvm-project 
cd llvm-project
git checkout llvmorg-20.1.3

mkdir build
mkdir install

cd build
cmake -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;compiler-rt;" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/path/to/gcc -DCMAKE_CXX_COMPILER=/path/to/g++ -DCMAKE_INSTALL_PREFIX=/path/to/llvm-project/install ../llvm

make -j64 && make -j64 install
```

then set the enviroment variables: 
```
export LD_LIBRARY_PATH=/path/to/llvm-project/install/lib:$LD_LIBRARY_PATH
export LIBCLANG=/path/to/llvm-project/install/lib
export PATH=/path/to/llvm-project/install/bin:$PATH
export CC=/path/to/llvm-project/install/bin/clang
export CXX==/path/to/llvm-project/install/bin/clang++
```

### (2) Z3
```
git clone https://github.com/Z3Prover/z3

cd z3
mkdir install
CXX=/path/to/clang++ CC=/path/to/clang python scripts/mk_make.py --prefix=/path/to/z3/install
cd build
make -j64 && make -j64 install
```

### (3) re2c
```
git clone https://github.com/skvadrik/re2c

cd re2c
mkdir .build && cd .build && cmake -DCMAKE_C_COMPILER=/path/to/clang -DCMAKE_CXX_COMPILER=/path/to/clang++ .. && cmake --build .

export PATH=/path/to/re2c/.build/:$PATH
```

### (4) alive2
```
git clone https://github.com/AliveToolkit/alive2

cd alive2
mkdir build
cd build

cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/path/to/clang -DCMAKE_CXX_COMPILER=/path/to/clang++ -DZ3_INCLUDE_DIR=/path/to/z3/install/include -DZ3_LIBRARIES=/path/to/z3/install/lib/libz3.so -DBUILD_TV=ON  ..

ninja -j64 
```
