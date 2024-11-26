## Test

We can run the full testsuite using `pytest`. Please make sure `pytest` is installed in the environment.
```
pip install pytest
pytest test.py
```

### Autotuner

#### Prerequisites
Install `opentuner` and `Autotuner` packages before running `loop-unroll` and `coarse-tuning` tests:
```
yum install Bisheng-opentuner
yum install Bisheng-Autotuner
```

Install `time` package and python `xgboost` package before running `option-tuning` test:
```
yum install time
pip install xgboost
```

#### loop-unroll
```bash
cd ./autotuner/loop_unroll
export PATH=/path/to/gcc12/bin:$PATH
bash run_autotuner.sh
```

#### coarse tuning (function-grained)
```bash
cd ./autotuner/coarse_tuning
export PATH=/path/to/gcc12/bin:$PATH
bash run_autotuner.sh
```

#### option tuning (application-grained)
```bash
cd ./option_tuner
export PATH=/path/to/gcc12/bin:$PATH
bash run_option_tuner.sh
```

### Compiler Optimization

Test compiler optimization plugin for gcc.

#### block-correction
```bash
cd ./optimizer/block_correction
export PATH=/path/to/gcc12/bin:$PATH
# You can define your own dynamic library path of AI4Compiler inference engine
# as `infer_engine_path` in `run_block_correction.sh`.
bash run_block_correction.sh
```