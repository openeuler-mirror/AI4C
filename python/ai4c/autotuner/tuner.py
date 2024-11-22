#!/usr/bin/env python3
# coding=utf-8
import argparse
import typing
import logging
import os
import glob

import opentuner

from ai4c.autotuner.utils import add_arg_trials
from ai4c.autotuner.utils import add_config_db_arguments
from ai4c.autotuner.utils import add_arg_search_space
from ai4c.autotuner.utils import add_arg_deterministic
from ai4c.autotuner.utils import add_code_region_filtering_arguments
from ai4c.autotuner.utils import add_use_dynamic_values
from ai4c.autotuner.utils import add_arg_baseline_config
from ai4c.autotuner.utils import add_func_instrument

logger = logging.getLogger(__name__)

MAX_PARALLELISM = 4096


class Tuner:
    import autotuner.utils as utils
    from autotuner.resumable.interface import AutoTunerInterface
    from autotuner.resumable.interface import StateSerializer
    from autotuner.iomanager import argument_parser as io_argument_parser

    def __init__(self, args: argparse.Namespace = None):
        self.trials: int = MAX_PARALLELISM

        self.data_dir = os.environ.get("AUTOTUNE_DATADIR", "autotune_datadir")
        os.makedirs(self.data_dir, exist_ok=True)

        self.args = args or self.get_args()
        self.command = self.args.command
        try:
            self.trials = self.args.trials or self.trials
        except AttributeError:
            pass

    def run(self):
        opentuner.init_logging()
        try:
            if self.command == "list":
                default_plugin_path = os.path.abspath(
                    os.path.join(os.path.dirname(__file__), "..", "lib"))
                plugins = glob.glob(f'{default_plugin_path}/*Plugin_gcc*.so') +\
                          glob.glob(f'{default_plugin_path}/*plugin_gcc*.so')
                print("- " + "\n- ".join(plugins))

                print("\n" + "---" * 20)
                print("# generate opportunities")
                print(
                    "$ <program> -fplugin-arg-<plugin-name>-generate=<opp-dir,"
                    " default is autotune_datadir/opp>")
                print("# tuning once")
                print(
                    "$ <program> -fplugin-arg-<plugin-name>-autotune=<test>/input.yaml\n"
                )
            elif self.command in ("minimize", "maximize"):
                self.initialize()
            elif self.command == "feedback":
                if self.args.feedback_file:
                    values = self.utils.parse_feedback_file(
                        self.args.feedback_file)
                elif self.args.values:
                    values = self.args.values
                else:
                    raise Exception("No performance feedback provided")
                self.feedback(values)
            elif self.command == "dump":
                self.dump()
            elif self.command == "finalize":
                self.finalize()
        except Exception as error:
            logger.error(error)
            logger.error("Executing command %s failed", self.command)
            exit(1)

    def initialize(self):
        state_serializer = self.StateSerializer(self.data_dir)
        state_serializer.check_state_exists()
        auto_tuner = self.AutoTunerInterface()
        auto_tuner.initialize(self.args, self.data_dir, self.command)
        auto_tuner.next_config(self.trials)
        state_serializer.serialize(auto_tuner)

    def feedback(self, feedback_numbers):
        state_serializer = self.StateSerializer(self.data_dir)
        auto_tuning_state = state_serializer.deserialize()
        auto_tuner = self.AutoTunerInterface()
        auto_tuner.resume(auto_tuning_state)
        auto_tuner.feedback(feedback_numbers)
        auto_tuner.next_config(self.trials)
        state_serializer.serialize(auto_tuner)

    def dump(self):
        state_serializer = self.StateSerializer(self.data_dir)
        auto_tuning_state = state_serializer.deserialize()
        auto_tuner = self.AutoTunerInterface()
        auto_tuner.resume(auto_tuning_state)
        auto_tuner.dump()

    def finalize(self):
        state_serializer = self.StateSerializer(self.data_dir)
        auto_tuning_state = state_serializer.deserialize()
        auto_tuner = self.AutoTunerInterface()
        auto_tuner.resume(auto_tuning_state)
        auto_tuner.finalize(self.args.config_update)

    def get_args(self) -> argparse.Namespace:
        top_parser = argparse.ArgumentParser(
            prog="ai4c-autotune",
            formatter_class=argparse.RawTextHelpFormatter)

        top_parser.add_argument('-v',
                                '--version',
                                action='version',
                                version='1.0.0')

        sub_parsers = top_parser.add_subparsers(dest="command")
        sub_parsers.required = True

        parent_parsers: typing.List[
            argparse.ArgumentParser] = opentuner.argparsers()
        parent_parsers.append(self.io_argument_parser)
        for parser in parent_parsers:
            for argument in parser._actions:
                argument.help = argparse.SUPPRESS

        sub_parsers.add_parser("list",
                               parents=parent_parsers,
                               help="Show gcc plugins list.")

        min_parser = sub_parsers.add_parser(
            "minimize",
            parents=parent_parsers,
            formatter_class=argparse.RawTextHelpFormatter,
            help="Initialize tuning and generate the "
            "initial compiler configuration file, "
            "aiming to minimize the metric "
            "(e.g. run time)")

        max_parser = sub_parsers.add_parser(
            "maximize",
            parents=parent_parsers,
            formatter_class=argparse.RawTextHelpFormatter,
            help="Initialize tuning and generate the "
            "initial compiler configuration file, "
            "aiming to maximize the metric "
            "(e.g. throughput)")
        for parser in (min_parser, max_parser):
            add_arg_trials(parser, self.trials)
            add_arg_search_space(parser)
            add_arg_deterministic(parser)
            add_config_db_arguments(parser)
            add_code_region_filtering_arguments(parser)
            add_use_dynamic_values(parser)
            add_arg_baseline_config(parser)

        feedback_parser = sub_parsers.add_parser(
            "feedback",
            formatter_class=argparse.RawTextHelpFormatter,
            help="Feed back performance tuning result(s) "
            "and generate new test configurations")

        add_arg_trials(feedback_parser, self.trials)

        sub_parsers.add_parser(
            "dump",
            formatter_class=argparse.RawTextHelpFormatter,
            help="Dump the current best configuration without "
            "terminating the tuning run")
        feedback_parser.add_argument("values",
                                     type=float,
                                     nargs='*',
                                     help="Performance tuning result(s)")
        feedback_parser.add_argument(
            "-i",
            "--feedback-file",
            help="Load feedback values from a CSV file; "
            "any values specified on command line are "
            "overridden by those specified in the file")

        finalize_parser = sub_parsers.add_parser(
            "finalize",
            formatter_class=argparse.RawTextHelpFormatter,
            help="Finalize tuning and generate the optimal "
            "compiler configuration")
        finalize_parser.add_argument(
            "--store-optimal-configs",
            dest="config_update",
            action="store_true",
            help="specifiy if the optimal configuration "
            "will be stored in configs.db upon "
            "completion. It will overwrite previous "
            "rows on conflict. Default: No update.")

        self.args = top_parser.parse_args()
        return self.args


def main():
    Tuner().run()


if __name__ == "__main__":
    main()
