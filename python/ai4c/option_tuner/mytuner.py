import os
import sys
import csv
import math
import argparse
import random
from typing import List
from random import randint
import logging
import numpy as np
import collections

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
# os.path.dirname => option_tuner dir

import utils
from measure import Measurer
from search import SimulatedAnnealingOptimizer
from xgboost_cost_model import XGBoostCostModel

logging.basicConfig(format='INFO: %(message)s',
                    level=logging.INFO,
                    handlers=[logging.StreamHandler(sys.stdout)])

enum_config_map = {}
ConfigfileResult = collections.namedtuple('ConfigfileResult', \
                        'type1 type2 type2_len type4 defaults minv maxv')


def read_config_file(option_file, lib_file):
    """ Reads configuration and library files, extracts options and values
        and returns relevant lists.

    Read the configuration file and extract the compiler options and
    Read the lib file and extract the lib options.

    Args:
        option_file (str): Configuration file path to compiler options
        lib_file (str): Configuration file path to library options

    Returns:
        type1 (list): A list of required_config and required_lib values.
        type2 (list): A list of bool_config and bool_lib values.
        len_type2 (int): The length of type2 list.
        type4 (list): A list of interval_config and enum_config names.
        defaults (list): A list of default values for interval_config options.
        minv (list): A list of minimum values for interval_config options.
        maxv (list): A list of maximum values for interval_config options.

    """
    # Read compiler option configuration files.
    content = utils.load_yaml_file(option_file)

    type_1 = []
    type_2 = []

    if 'required_config' in content and content['required_config'] is not None:
        type_1 = content['required_config']

    if 'bool_config' in content and content['bool_config'] is not None:
        type_2 = content['bool_config']

    defaults = []
    minv = []
    maxv = []
    type4 = []

    if 'interval_config' in content and content['interval_config'] is not None:
        for t_4 in content['interval_config']:
            type4.append(t_4['name'])
            defaults.append(t_4['default'])
            minv.append(t_4['min'])
            maxv.append(t_4['max'])

    if 'enum_config' in content and content['enum_config'] is not None:
        for t4s in content['enum_config']:
            type4.append(t4s['name'])
            defaults.append(0)
            minv.append(0)
            maxv.append(len(t4s['options']) - 1)

            enum_config_map[t4s['name']] = []

            for option in t4s['options']:
                enum_config_map[t4s['name']].append(option)

    # Read dynamic library configuration files.
    content_lib = utils.load_yaml_file(lib_file)
    if 'required_lib' in content and content['required_lib'] is not None:
        type_1 += content['required_lib']
    if 'bool_lib' in content and content['bool_lib'] is not None:
        type_2 += content['bool_lib']

    return ConfigfileResult(type_1, type_2, len(type_2), type4, defaults, minv,
                            maxv)


class Tuner(object):

    def __init__(self, n_parallel, n_trial, plan_size, cost_model,
                 model_optimizer, option_config, measurer):
        # space
        self.n_parallel = n_parallel
        self.plan_size = plan_size
        self.required_config, self.chooseconfig = \
            option_config.type1, option_config.type2
        self.t4, self.t4_defaults, self.t4_minv, self.t4_maxv = \
            option_config.type4, option_config.defaults, \
            option_config.minv, option_config.maxv

        # cost_model
        self.total = 0
        self.cost_model = cost_model

        # search_model
        self.model_optimizer = model_optimizer

        # trial plan
        self.trials = []
        self.trial_pt = 0
        self.visited = set()

        # observed samples
        self.xs = []
        self.ys = []
        self.train_ct = 0

        # keep the current best
        self.measurer = measurer
        self.best_config = None
        self.best_res = 1e9
        self.best_measure_pair = None
        self.best_res_display = self.best_res

        # time to leave
        self.ttl = None
        self.n_trial = n_trial
        self.early_stopping = None

    def get_random_config(self):
        return [randint(0, 1) for i in range(len(self.chooseconfig))]

    def get_random_config_t4(self):
        return [
            randint(int(self.t4_minv[i]), int(self.t4_maxv[i]))
            for i in range(len(self.t4))
        ]

    def next_batch(self, batch_size):
        """ The function generates a batch of compiler options for tuning, 
            ensuring uniqueness and specified batch size.

        Generate the next batch of compiler options for tuning.

        Args:
            batch_size (int): The number of compiler options to generate.

        Returns:
            ret (list): A list of compiler options.

        """
        ret = []
        counter = 0
        while counter < batch_size:
            while self.trial_pt < len(self.trials):
                config = self.trials[self.trial_pt]
                if hash(str(config)) not in self.visited:
                    break
                self.trial_pt += 1

            # If the trial list is empty or the tuner is doing the last 5%
            # trials, choose randomly.
            if self.trial_pt >= len(self.trials) - int(0.05 * self.plan_size):
                config = self.get_random_config() + self.get_random_config_t4()
                while hash(str(config)) in self.visited:
                    config = self.get_random_config(
                    ) + self.get_random_config_t4()

            ret.append(config)
            self.visited.add(hash(str(config)))

            counter += 1
        return ret

    def input2compile(self, inputs: List[int]):
        """ The function converts input values into a string of compiler 
            options for tuning.

        Convert the input values to a compiler options string.

        Args:
        inputs (list): A list of input values.

        Returns:
        compile_line (str): A string of compiler options.

        """
        initial_compile_line = ""
        for cconfig in self.required_config:
            initial_compile_line += cconfig + ' '
        compile_line = initial_compile_line
        for i, cconfig in enumerate(self.chooseconfig):
            if inputs[i]:
                compile_line += cconfig + ' '
        length = len(self.chooseconfig)
        for i, tt4 in enumerate(self.t4):
            if tt4 not in enum_config_map:
                compile_line += tt4 + "=%s " % (inputs[i + length])
            else:
                compile_line += tt4 + \
                    "=%s " % (enum_config_map.get(tt4)[inputs[i + length]])
        return compile_line

    def measure_batch(self, inputs: List[List[int]]):
        """ measure performance

        Measure the performance of a batch of compiler options.

        Args:
            inputs (list): A list of input values.

        Returns:
            ret (list): A list of performance measurements.

        """
        rets = []
        for inp in inputs:
            compile_line = self.input2compile(inp)
            ret = self.measurer.measure_once(compile_line, self.total)
            rets.append(ret)
            self.total += 1

            ret_display_val = 0
            if float(ret) != float('-inf'):
                ret_display_val = math.exp(ret)
            if 0 < ret_display_val < self.best_res_display:
                self.best_res_display = ret_display_val

            logging.info(f"\rIteration {self.total}/{self.n_trial}: "
                         f"{ret_display_val:.3f}, "
                         f"Current Best Result: {self.best_res_display:.3f}")
        return rets

    def update(self, inputs: List[List[int]], results: List[float]):
        """ update xgb

        Update the tuner and the xgb cost model with the new inputs and results

        Args:
            inputs (list): A list of input values.
            results (list): A list of performance measurements.

        Returns:

        """
        for inp, res in zip(inputs, results):
            if math.isinf(res):
                continue
            self.xs.append(inp)
            self.ys.append(res)
            self.visited.add(hash(str(inp)))

        if len(self.xs) >= self.plan_size * (self.train_ct + 1):
            self.cost_model.fit(self.xs, self.ys)
            maximums = self.model_optimizer.find_maxima(
                self.cost_model, self.plan_size, self.visited)

            self.trials = maximums
            self.trial_pt = 0
            self.train_ct += 1

    def tune(self):
        """ tune process

        Perform the tuning process.

        """
        i = 0
        while i < self.n_trial:
            inputs = self.next_batch(min(self.n_parallel, self.n_trial - i))
            results = self.measure_batch(inputs)
            for inp, res in zip(inputs, results):
                if res < self.best_res:
                    self.best_res = res
                    compile_line = self.input2compile(inp)
                    self.best_config = compile_line
                    self.best_measure_pair = (compile_line, res)

            i += len(results)
            self.update(inputs, results)


def main():
    mytuner_dir = os.path.dirname(os.path.abspath(__file__))

    parser = argparse.ArgumentParser(description='Auto Tuner argparse')
    parser.add_argument('--test_limit',
                        help='total number of trials',
                        default=300)
    parser.add_argument('--use_random_seed',
                        help='option to set random seed',
                        default=False)
    parser.add_argument('--optionfile',
                        '-c',
                        help='config file path of tuning options (relative)',
                        default=utils.create_full_path(mytuner_dir,
                                                       './input/options.yaml'))
    parser.add_argument(
        '--libfile',
        '-l',
        help='config file path of dynamic libraries (relative)',
        default=utils.create_full_path(mytuner_dir,
                                       './input/options_lib.yaml'))
    parser.add_argument('--measurefile',
                        '-m',
                        help='config file path of different measure weights '
                        'in objective function (relative)',
                        default=utils.create_full_path(
                            mytuner_dir, './input/config_measure.yaml'))
    parser.add_argument('--runfile',
                        '-r',
                        help='compilation and execution script (absolute)',
                        required=True)

    curr_wkdir = os.getcwd()
    args = parser.parse_args()

    if bool(args.use_random_seed):
        # set up random seed
        random.seed(42)

    configs = read_config_file(args.optionfile, args.libfile)
    type1, type2, search_len = configs.type1, configs.type2, configs.type2_len
    t4, t4_defaults, t4_minv, t4_maxv = configs.type4, \
        configs.defaults, configs.minv, configs.maxv

    # Clear all intermediate files in `./tuning` and `./result`.
    tuning_dir = os.path.join(curr_wkdir, "tuning")
    utils.create_empty_directory(tuning_dir)
    perform_file = os.path.join(tuning_dir, "performance.txt")
    config_file = os.path.join(tuning_dir, "config.txt")

    result_dir = os.path.join(curr_wkdir, "result")
    utils.create_empty_directory(result_dir)
    result_file = os.path.join(tuning_dir, "result.csv")
    train_file = os.path.join(tuning_dir, "train.csv")

    xgb = XGBoostCostModel()
    sa = SimulatedAnnealingOptimizer(4, t4, t4_defaults, t4_minv, t4_maxv,
                                     search_len)
    measurer = Measurer(args.runfile, args.measurefile, perform_file,
                        config_file, train_file)
    mytuner = Tuner(n_parallel=8,
                    n_trial=int(args.test_limit),
                    plan_size=16,
                    cost_model=xgb,
                    model_optimizer=sa,
                    option_config=configs,
                    measurer=measurer)
    mytuner.tune()

    logging.info(f"Best Result: {mytuner.best_res_display}")
    logging.info(f"Best Config: {mytuner.best_config}")

    with open(result_file, 'w') as f:
        csv_writer = csv.writer(f)
        csv_writer.writerow(mytuner.best_measure_pair)


if __name__ == '__main__':
    main()