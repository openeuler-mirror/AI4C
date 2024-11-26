# AI4C Python Toolkit

This is a Python toolkit that contains all functional modules of the AI4Compiler framework.

## Function List

[1. Autotuner](docs/autotuner.md)
[2. GCC AI-Enabled-Optimization](docs/gcc-opt.md)

## Installation

Install AI4Compiler framework from source code.

```sh
git clone https://gitee.com/openeuler/AI4C.git
cd ./python
python3 -m pip install .
```

If you want to use AI4C Autotuner, please install `BiSheng-opentuner` and `BiSheng-Autotuner` packages.

- BiSheng-opentuner
https://gitee.com/src-openeuler/BiSheng-opentuner/blob/master/bisheng-opentuner.zip

```sh
unzip bisheng-opentuner.zip
cd BiSheng-opentuner-0.8.8
python3 -m pip install .
```

- BiSheng-Autotuner

```sh
git clone https://gitee.com/openeuler/BiSheng-Autotuner
cd BiSheng-Autotuner
python3 -m pip install .
```
