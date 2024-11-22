# export PATH=/path/to/gcc12/bin:$PATH
import subprocess as sp


def test_autotune_loop_unroll():
    sp.check_call(["bash", "run_autotuner.sh", "clean"],
                  cwd="autotuner/loop_unroll")


def test_autotune_coarse_tuning():
    sp.check_call(["bash", "run_autotuner.sh", "clean"],
                  cwd="autotuner/coarse_tuning")


def test_optimizer_block_correction():
    sp.check_call(["bash", "run_block_correction.sh", "clean"],
                  cwd="optimizer/block_correction")


def test_option_tuner():
    sp.check_call(["bash", "run_option_tuner.sh", "clean"], cwd="option_tuner")
