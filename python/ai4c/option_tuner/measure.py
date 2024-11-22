import os
import csv
import math
from typing import List
import logging
import sys
import subprocess as sp

from utils import load_yaml_file

logging.basicConfig(format='INFO: %(message)s',
                    level=logging.INFO,
                    handlers=[logging.StreamHandler(sys.stdout)])


class Measurer(object):

    def __init__(self, run_file_, measure_file_, perform_file_, config_file_,
                 train_file_):
        self.run_file = run_file_
        self.measure_file = measure_file_
        self.perform_file = perform_file_
        self.config_file = config_file_
        self.train_file = train_file_

        self.measure_list = None

        content = load_yaml_file(self.measure_file)
        if 'config_measure' in content and \
            content['config_measure'] is not None:
            self.measure_list = content['config_measure']

        with open(self.train_file, 'w') as f:
            csv_writer = csv.writer(f)
            csv_writer.writerow(['config', 'out', 'performance'])

        self.train_data = []

    def write_to_train_file(self):
        with open(self.train_file, 'a+') as f:
            csv_writer = csv.writer(f)
            csv_writer.writerow(self.train_data)

    def write_tuningconfig_to_file(self, config):
        if os.path.exists(self.config_file):
            os.remove(self.config_file)
        with open(self.config_file, 'w') as f:
            f.write(config)

    def _execute_file(self, file_path):
        try:
            result = sp.run(["sh", file_path, os.getcwd()],
                            capture_output=True,
                            text=True)

            # Check if the command was successful
            if result.returncode != 0:
                print("Output:\n", result.stdout)
                print("Error:\n", result.stderr)

        except FileNotFoundError:
            print(f"File not found: {file_path}")
        except sp.CalledProcessError as e:
            print(f"Command failed with exit code {e.returncode}: {e.stderr}")
        except Exception as e:
            print(f"An error occurred: {e}")

    def compile_and_run(self):
        if os.path.exists(self.perform_file):
            os.remove(self.perform_file)
        os.chmod(self.run_file, int("0o755", 8))
        self._execute_file(self.run_file)

    def calculate_performance(self, performances: List[float]):
        """
        A function interface that takes multiple-dimensional performance values 
        as input and outputs a single value (should add logic below).

        The function takes multiple-dimensional performance values as input and 
        computes a single output value. 
        It applies a specific logic to calculate the performance based on the 
        input measurements. 
        The function uses logarithmic calculations and parameter weights to 
        determine the final performance value. 
        The calculated performance is returned as a negative value 
        (because the larger the performance, the better)
        
        Args:
            performances (list): A list of performance measurements.
    
        Returns:
            calc_performance (float): The output computed from the input

        """
        length = len(performances)

        if self.measure_list is None:
            self.measure_list = [1] * length

        if len(self.measure_list) != length:
            raise ValueError("Make sure the number of measures in "
                             "config_measure.yaml agrees with that of "
                             "extracted performances meansures in execution "
                             "scripts.")

        performance = 0.0
        for i in range(length):
            measure = self.measure_list[i]
            if type(measure) is dict and 'optim' in measure:
                log_perf = 0.0
                if performances[i] > 0.0:
                    log_perf = math.log(performances[i])
                else:
                    log_perf = -1e-10
                if measure['optim'] == 'maximize':
                    performance += measure['weight'] * log_perf
                else:
                    performance -= measure['weight'] * log_perf
            else:
                performance += measure * math.log(performances[i])

        return -performance

    def get_performance(self):
        if os.path.exists(self.perform_file):
            with open(self.perform_file, 'r') as f:
                lines = f.readlines()

                performances = [float(x) for x in lines[0].split()]
                self.train_data.append(lines[0].strip())

            performance = self.calculate_performance(performances)
        else:
            logging.info("WARNING: Compile error! Find details in " +
                         "`tuning/train.csv`.")
            performance = float('-inf')

        return performance

    def measure_once(self, param_config, param_id):
        """ The function measures the performance of a single configuration 
        by compiling, running, and obtaining the performance measurement.

        Measure the performance of a single configuration.
        First write the compile_config to tuning/config.txt
        Then compile and run
        Finally get the performance (may handle_data) and write to log file 

        Args:
            config (str): The configuration to measure.
            id (int): The ID of the measurement.
        
        Returns:
            p (float): The performance measurement.

        """
        self.train_data = []

        self.write_tuningconfig_to_file(param_config)
        self.train_data.append(param_config)

        self.compile_and_run()
        p = float(self.get_performance())

        self.train_data.append(p)
        self.write_to_train_file()

        return p


if __name__ == '__main__':
    m = Measurer('./input/run.sh', './input/config_measure.txt')
    m.measure_once("-O2", 333)
