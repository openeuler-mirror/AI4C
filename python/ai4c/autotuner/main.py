#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import argparse
import logging
import glob
import os
import imp
import sys
import enum
from collections import OrderedDict
from datetime import datetime

from configparser import Error as ConfigParserError
from configparser import ConfigParser

import opentuner
from opentuner import ConfigurationManipulator
from opentuner import MeasurementInterface
from opentuner import Result

from autotuner.tuners import tunerbase
from autotuner import utils
from autotuner.iomanager import EmptySearchSpaceError
from autotuner.iomanager import argument_parser
from autotuner.iomanagerutils import create_io_manager

from ai4c.autotuner.utils import comfirm_security
from ai4c.autotuner.utils import add_config_db_arguments
from ai4c.autotuner.utils import get_remarks, get_results
from ai4c.autotuner.utils import add_func_instrument

logger = logging.getLogger(__name__)


class STAGE(enum.Enum):
    module = 'module'
    function = 'function'
    loop = 'loop'
    machine_basic_block = 'machine_basic_block'


class TunerBase(MeasurementInterface):

    def __init__(self,
                 args,
                 compile_dir,
                 config_file,
                 search_space,
                 compile_cmd,
                 fixed_config_files=None,
                 enable_final_compile=False,
                 stage=None,
                 config_db=None,
                 *pargs,
                 **kwargs):
        super(TunerBase, self).__init__(args, *pargs, **kwargs)
        self.iomanager = create_io_manager(args.parse_format)
        self.compile_dir = compile_dir
        self.config_file = config_file
        self.compile_cmd = compile_cmd
        self.task_map = self.iomanager.parse_search_space(search_space)
        self.config_db = config_db
        self.use_hash_matching = args.use_hash_matching

        if fixed_config_files:
            self.fixed_llvm_config_tree = self.iomanager.parse_llvm_inputs(
                fixed_config_files)
        else:
            self.fixed_llvm_config_tree = None

        self.enable_final_compile = enable_final_compile

        if args.time_after_convergence:
            self.last_best_result_time = datetime.now()
        if args.output:
            self.output_dir = args.output
        else:
            self.output_dir = ""
        if stage and not stage in STAGE.__members__:
            raise Exception("Illegal stage: " + stage)
        self.stage = stage
        self.verbose = args.verbose

    def manipulator(self):
        manipulator = ConfigurationManipulator()
        for _, task in self.task_map.items():
            for param in task.param_list:
                manipulator.add_parameter(param)
        return manipulator

    def compile(self, config_data=None, compile_id=None):
        compile_dir = None if 'ai4c-autotune' in self.compile_cmd \
                           else self.compile_dir
        compile_result = self.call_program(self.compile_cmd,
                                           cwd=compile_dir,
                                           limit=1500)
        return compile_result

    def compile_and_run(self, desired_result, desired_input, limit):
        cfg = desired_result.configuration.data
        self.iomanager.build_llvm_input(cfg, self.task_map, self.config_file,
                                        self.fixed_llvm_config_tree,
                                        self.config_db, self.use_hash_matching)

        compile_result = self.compile()
        if self.verbose:
            print(compile_result['stdout'])
        if compile_result['timeout']:
            print("compiling timeoout")
            return Result(state='TIMEOUT', time=float('inf'))
        elif compile_result['returncode'] != 0:
            print("compiling error, test failed")
            print(compile_result["stderr"])
        else:
            return self.run(desired_result, desired_input, limit)

        return Result(state='ERROR',
                      time=float('inf'),
                      cycle=float('inf'),
                      rate=-float('inf'))

    def extra_convergence_criteria(self, result_list):
        if self.args.time_after_convergence:
            is_any_new_best = any([ele.was_new_best for ele in result_list
                                   ]) if result_list else False
            if is_any_new_best:
                self.last_best_result_time = datetime.now()
            else:
                elapsed = (datetime.now() - self.last_best_result_time).seconds
                if elapsed > self.args.time_after_convergence:
                    print("time elapsed since last best result found: " +
                          str(elapsed))
                    return True
        return False

    def save_final_config(self, configuration):
        logger.info("Tuning run is ending...")
        if self.enable_final_compile:
            logger.info("Performing final compilation with opt config...")
            self.iomanager.build_llvm_input(configuration.data, self.task_map,
                                            self.config_file,
                                            self.fixed_llvm_config_tree,
                                            self.config_db,
                                            self.use_hash_matching)
            compile_result = self.compile()
            if compile_result['returncode'] != 0:
                logger.info("Compiling error")
                logger.info(compile_result["stderr"])
            else:
                logger.info("Final compilation succeeded")

        output_name = self.stage if self.stage else "opt_config"
        output_path = os.path.join(self.output_dir, output_name)

        if self.config_db:
            self.iomanager.update_config_db(
                configuration.data,
                self.task_map,
                config_db=self.config_db,
                use_hash_matching=self.use_hash_matching)
            logger.info(
                "configs.db has been updated with optimal configurations.")

        self.iomanager.build_llvm_input(
            configuration.data, self.task_map,
            output_path + self.iomanager.get_file_extension(),
            self.fixed_llvm_config_tree, self.config_db,
            self.use_hash_matching)
        logger.info("Optimal configuration for llvm/clang has been saved to " +
                    output_path + self.iomanager.get_file_extension())
        self.manipulator().save_to_file(configuration.data,
                                        output_path + ".json")
        logger.info(
            "Optimal json configuration for opentuner has been saved to " +
            output_path + ".json")
        logger.info("You can use the json file with --seed-configuration "
                    "for next tuning run")

    def print_errors(self, cmd, run_result):
        logger.info('running command failed, the error was: ')
        logger.info(run_result['stderr'])
        logger.info('the cmd was: ')
        logger.info(cmd + '\n')


class CustomTunerBase(TunerBase):

    def __init__(self, args, compile_dir, config_file, search_space,
                 compile_cmd, run_dir, run_cmd, *pargs, **kwargs):
        super(CustomTunerBase,
              self).__init__(args, compile_dir, config_file, search_space,
                             compile_cmd, *pargs, **kwargs)
        self.run_dir = run_dir
        self.run_cmd = run_cmd


class SimpleTuner(CustomTunerBase):

    def run(self, desired_result, desired_input, limit):
        time = float('inf')
        run_result = self.call_program(self.run_cmd,
                                       cwd=self.run_dir,
                                       limit=limit)
        if run_result['returncode'] == 0:
            time = run_result['time']
        else:
            self.print_errors(self.run_cmd, run_result)
        return Result(time=time)


class AutoTuner:

    def __init__(self):
        self.functions = dict(
            merge=self.merge_main,
            divide=self.divide_main,
            parse=self.parse_main,
            run=self.run_main,
            autorun=self.auto_run_main,
            dump=self.dump,
        )

    def run(self):
        self.args = self.get_args()
        self.command: str = self.args.command

        if self.command not in self.functions:
            logger.error(f"Unknown command: {self.command}")
            exit(1)
        try:
            self.functions[self.command]()
            exit(0)
        except EmptySearchSpaceError:
            logger.error('Empty search space, stop tuning')
        except ConfigParserError as error:
            logger.error('Failed to parse your configuration file: ' +
                         self.args.config_file)
            logger.error(error)
        except (OSError, IOError) as error:
            logger.error('Failed to execute command "' + self.command + '". ')
            logger.error(error)
        except Exception as error:
            logger.exception('Failed to execute command "' + self.command +
                             '". ')
        exit(1)

    def get_args(self) -> argparse.Namespace:
        parser = argparse.ArgumentParser(prog="AI4c AutoTuner")
        subparsers = parser.add_subparsers(help="commands help",
                                           dest="command")
        subparsers.required = True

        argparsers = opentuner.argparsers()
        argparsers.append(tunerbase.argument_parser)
        argparsers.append(argument_parser)

        run_parser = subparsers.add_parser("run",
                                           parents=argparsers,
                                           help="Run the tuner")
        self.add_common_tuner_arguments(run_parser)
        run_parser.add_argument(
            "-cg",
            "--coarse-grained",
            action="store_true",
            help="Coarse-grained tuning (compilation options).")
        run_parser.add_argument("-ss",
                                "--search_space",
                                help="The search space file.")
        run_parser.add_argument("--enable-final-compile",
                                action="store_true",
                                default=False,
                                help="perform final compilation with optimal "
                                "config at the end of tuning")

        merge_parser = subparsers.add_parser(
            "merge",
            parents=[argument_parser],
            help="Merge configuration input files")
        merge_parser.add_argument("input_file",
                                  nargs="+",
                                  help="Configuration input files")
        merge_parser.add_argument("-o",
                                  "--output",
                                  metavar="FILE",
                                  help="output file")

        divide_parser = subparsers.add_parser(
            "divide",
            parents=[argument_parser],
            help="Divide configuration input file into "
            "multiple files based on file_name")
        divide_parser.add_argument("input_file",
                                   help="Configuration input file")
        divide_parser.add_argument("-o",
                                   "--output_dir",
                                   default='./',
                                   metavar="DIR",
                                   help="output dir")

        gsc_parser = subparsers.add_parser(
            "parse",
            parents=[argument_parser],
            help="Parse the tuning opportunity files and "
            "generate search space")

        self.add_common_parse_arguments(gsc_parser)
        gsc_parser.add_argument('opp_file',
                                nargs='+',
                                help="Opportunity files generated by compiler")
        gsc_parser.add_argument("-o",
                                "--output",
                                metavar="FILE",
                                help="output file")
        gsc_parser.add_argument(
            "-tf",
            "--type-filter",
            nargs="+",
            default=[],
            help="to filter code regions by types when"
            " generating search space",
            choices=["machine_basic_block", "loop", "function", "module"])

        auto_run_parser = subparsers.add_parser(
            "autorun",
            parents=argparsers,
            help="(recommended) auto-generate the search "
            "space and run the auto-phase-based tuning "
            "(the default order of stages is module -> "
            "function -> loop)")
        auto_run_parser.add_argument(
            "--stage-order",
            nargs="+",
            metavar="stage",
            default=["module", "function", "loop"],
            help="specify stage order of autorun. "
            "each stage is a code region type",
            choices=["machine_basic_block", "function", "loop", "module"])

        auto_run_dumper = subparsers.add_parser("dump",
                                                parents=argparsers,
                                                help="dump autotune results")
        auto_run_dumper.add_argument("-c",
                                     "--config_file",
                                     help="input config file (.yaml)")

        self.add_common_parse_arguments(auto_run_parser)
        self.add_common_tuner_arguments(auto_run_parser)

        args = parser.parse_args()
        return args

    def add_common_tuner_arguments(self, parser: argparse.ArgumentParser):
        add_config_db_arguments(parser)
        parser.add_argument("config_file",
                            type=comfirm_security,
                            help="The tuning config file.")
        parser.add_argument("--plugin-dir",
                            metavar="DIR",
                            help="specify the dir to load "
                            "customized tuner scripts")
        parser.add_argument("-tr",
                            "--tuner",
                            type=str,
                            help="Select which tuner to use")
        parser.add_argument("-lr",
                            "--list-tuners",
                            action="store_true",
                            help="List all available tuners")
        parser.add_argument(
            "--add-llvm-inputs",
            nargs="+",
            help="add existing llvm configuration input files as "
            "constants in addition to the llvm configurations"
            " generated in each iteration of the tuning run")
        parser.add_argument("-cfi",
                            "--create-func-instrument",
                            help="create function level instrumentation.")
        parser.add_argument("-afi",
                            "--add-func-instrument",
                            help="add function level instrumentation.")
        parser.add_argument("--instrument-exclude-file-list",
                            help="Set the list of functions that are excluded "
                            "from instrumentation (see the description of "
                            "@option{-finstrument-functions}). ")

    def add_common_parse_arguments(self, parser: argparse.ArgumentParser):
        parser.add_argument(
            "-nf",
            "--name-filter",
            nargs="+",
            metavar="Name",
            default=[],
            help="to filter code regions by names when generating "
            "search space")
        parser.add_argument(
            "--func-name-filter",
            nargs="+",
            metavar="Name",
            default=[],
            help="to filter code regions by function names when "
            "generating search space")
        parser.add_argument("--file-name-filter",
                            nargs="+",
                            metavar="Name",
                            default=[],
                            help="to filter code regions by file names when "
                            "generating search space")
        parser.add_argument("--hot-func-file",
                            nargs="+",
                            metavar="Name",
                            default=[],
                            help=argparse.SUPPRESS)
        parser.add_argument("--hot-func-number",
                            metavar="N",
                            type=int,
                            default=10,
                            help=argparse.SUPPRESS)
        parser.add_argument("-scf",
                            "--search-config-file",
                            help="The Search space config file")
        parser.add_argument("-v",
                            "--verbose",
                            action="store_true",
                            help="Enable verbose output")

    def merge_main(self):
        iomanager = create_io_manager(self.args.parse_format)
        if self.args.input_file:
            input_files = self.args.input_file
            if self.args.output:
                output_file = self.args.output
            else:
                output_file = "merged_input" + iomanager.get_file_extension()
            logger.info("Merging configuration input files: " +
                        str(input_files))
            content = iomanager.parse_llvm_inputs(input_files)
            iomanager.output_to_file(output_file, content)
            logger.info("The merged file has been generated: " + output_file)
            sys.exit(0)

    def divide_main(self):
        iomanager = create_io_manager(self.args.parse_format)
        input_file = self.args.input_file
        output_dir = os.path.realpath(self.args.output_dir)
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)

        logger.info("Dividing configuration input file: " + str(input_file))
        file_map = iomanager.divide_llvm_input(input_file)
        for file_name in file_map:
            basename = os.path.basename(
                file_name) + iomanager.get_file_extension()
            output_path = os.path.join(output_dir, basename)
            iomanager.output_to_file(output_path, file_map[file_name])
        logger.info("The divided files have been generated under: " +
                    output_dir)
        sys.exit(0)

    def parse_main(self):
        iomanager = create_io_manager(self.args.parse_format)
        if not self.args.search_config_file:
            dir_name = os.path.dirname(__file__)
            self.args.search_config_file = os.path.join(
                dir_name, "search_space_config/default_search_space" +
                iomanager.get_file_extension())
        if self.args.opp_file:
            input_files = self.args.opp_file
            if self.args.output:
                output_file = self.args.output
            else:
                output_file = "search_space" + iomanager.get_file_extension()

            if self.args.hot_func_file:
                hot_function_list = utils.parse_hot_function(
                    self.args.hot_func_file, self.args.hot_func_number)
                self.args.func_name_filter = list(
                    set(hot_function_list).union(
                        set(self.args.func_name_filter)))

            logger.info("Generating search space from " + str(input_files))
            iomanager.generate_search_space_file(input_files, output_file,
                                                 self.args.search_config_file,
                                                 self.args.name_filter,
                                                 self.args.func_name_filter,
                                                 self.args.file_name_filter,
                                                 self.args.type_filter)
            logger.info("The search space has been generated: " + output_file)
            sys.exit(0)

    def parse_common_options(self, args: argparse.Namespace):
        if args.list_tuners:
            logger.info("Available tuners: " +
                        str(tunerbase.get_available_tuners(args.plugin_dir)))
            sys.exit(0)

        if args.list_techniques:
            techniques, _ = opentuner.search.technique.all_techniques()
            for technique in techniques:
                logger.info(technique.name)
            sys.exit(0)

        if not args.config_file:
            logger.error("Auto-tuner run: error: config file is required")
            sys.exit(1)

        if args.command == "run" and not args.search_space:
            logger.error(
                "Auto-tuner run: error: argument -ss/--search_space is required"
            )
            sys.exit(1)

        if args.tuner:
            from importlib import import_module
            try:
                if args.plugin_dir:
                    if not os.path.isdir(args.plugin_dir):
                        raise IOError("Error: " + args.plugin_dir +
                                      " not found")
                    module_file = os.path.join(args.plugin_dir,
                                               args.tuner + ".py")
                    if not os.path.isfile(module_file):
                        raise IOError("Error: " + module_file + " not found")
                    module = imp.load_source(args.tuner, module_file)
                else:
                    module = import_module("autotuner.tuners." + args.tuner)
                tuner = getattr(module, 'Tuner')
            except ImportError as error:
                logger.error(error)
                logger.error("Please select a valid tuner name.")
                logger.error(
                    "Available tuners: " +
                    str(tunerbase.get_available_tuners(args.plugin_dir)))
                sys.exit(1)
        else:
            tuner = SimpleTuner

        if args.output:
            if not os.path.isdir(args.output):
                try:
                    os.mkdir(args.output)
                except OSError as error:
                    raise error

        if not os.path.isfile(args.config_file):
            raise IOError("Error: config file not found")

        utils.check_file_permissions(args.config_file)

        config = ConfigParser()
        config.optionxform = str
        config["DEFAULT"]["PluginPath"] = os.path.abspath(
            os.path.join(os.path.dirname(__file__), "..", "lib"))
        config["DEFAULT"]["TuningYAMLFile"] = os.path.abspath(
            os.path.join(os.path.dirname(__file__), "..",
            "autotuner/yaml/coarse_options.yaml"))
        config["DEFAULT"]["ConfigFilePath"] = os.path.abspath(
            os.path.dirname(args.config_file))
        config.read(args.config_file)
        if config.has_section("Environment Setting"):
            self.setup_env(config["Environment Setting"])
        return tuner, config

    def setup_env(self, environment_section):
        for (key, value) in list(environment_section.items()):
            path_list = [
                os.path.expanduser(path.strip()) for path in value.split(",")
            ]
            if key in os.environ:
                os.environ[key] = os.pathsep.join(path_list) \
                    + os.pathsep + os.environ[key]
            else:
                os.environ[key] = os.pathsep.join(path_list)

    def clean_opp(self, opp_dir):
        files = glob.glob(opp_dir + "/*")
        for ele in files:
            if os.path.isfile(ele):
                os.remove(ele)

    def run_main(self):
        tuner, config = self.parse_common_options(self.args)
        compile_section = config["Compiling Setting"]
        config_file = os.path.expanduser(compile_section["LLVMInputFile"])
        compile_dir = os.path.expanduser(compile_section["CompileDir"])
        compile_cmd = compile_section["CompileCommand"]

        if self.args.create_func_instrument is not None:
            self.create_func_instrument(compile_dir)
            exit(0)

        if self.args.coarse_grained:
            compile_cmd = self.generate_cl_options_config(
                config_file, compile_cmd, compile_dir)

        opentuner.init_logging()

        if self.args.add_func_instrument:
            compile_cmd = self.add_func_instrument(compile_cmd, compile_dir)

        tuner.main(self.args,
                   compile_dir=compile_dir,
                   program_name=self.args.config_file,
                   config_file=config_file,
                   fixed_config_files=self.args.add_llvm_inputs,
                   enable_final_compile=self.args.enable_final_compile,
                   search_space=self.args.search_space,
                   run_dir=compile_section["RunDir"],
                   run_cmd=compile_section["RunCommand"],
                   compile_cmd=compile_cmd)

    def auto_run_main(self):
        iomanager = create_io_manager(self.args.parse_format)
        is_first_stage = True
        if not self.args.search_config_file:
            dir_name = os.path.dirname(__file__)
            self.args.search_config_file = os.path.join(
                dir_name, "search_space_config/default_search_space" +
                iomanager.get_file_extension())
        tuner, config = self.parse_common_options(self.args)

        compile_section = config["Compiling Setting"]
        config_file = os.path.expanduser(compile_section["LLVMInputFile"])
        compile_dir = os.path.expanduser(compile_section["CompileDir"])
        compile_cmd = compile_section["CompileCommand"]
        run_cmd = compile_section["RunCommand"]
        run_dir = compile_section["RunDir"]

        compile_dir = os.path.abspath(compile_dir)
        run_dir = os.path.abspath(run_dir)
        if self.args.create_func_instrument is not None:
            self.create_func_instrument(compile_dir)
            exit(0)

        opp_compile_cmd = compile_section["OppCompileCommand"]
        opp_dir = os.path.expanduser(compile_section["OppDir"])

        opentuner.init_logging()

        if self.args.add_func_instrument is not None:
            compile_cmd = self.add_func_instrument(compile_cmd, compile_dir)

        os.makedirs(os.path.dirname(config_file), exist_ok=True)
        try:
            os.remove(config_file)
        except OSError:
            pass

        stages = list(OrderedDict.fromkeys(self.args.stage_order))

        stages_info_str = ", ".join(stages)

        logger.info("Running tuning with the stage order: {:s}".format(
            stages_info_str))

        for index, phase in enumerate(stages):
            if is_first_stage:
                fixed_config_files = self.args.add_llvm_inputs
                iomanager.create_dummy_llvm_input(config_file)
            else:
                fixed_config_files = [config_file]

            logger.info(
                "=== Starting stage {:d}: {:s} level tuning ===".format(
                    index + 1, phase))
            search_space_tree = self.generate_search_space(
                compile_dir, iomanager, opp_compile_cmd, opp_dir, phase)
            if search_space_tree is None:
                break
            try:
                tuner.main(self.args,
                           compile_dir=compile_dir,
                           config_file=config_file,
                           enable_final_compile=True,
                           fixed_config_files=fixed_config_files,
                           search_space=search_space_tree,
                           run_dir=run_dir,
                           run_cmd=run_cmd,
                           compile_cmd=compile_cmd,
                           stage=phase)
                is_first_stage = False
            except EmptySearchSpaceError:
                logger.error('Empty search space, stop the current stage')

    def generate_search_space(self, compile_dir, iomanager, opp_compile_cmd,
                              opp_dir, phase):
        opp_dir = os.path.join(compile_dir, opp_dir)
        os.makedirs(opp_dir, exist_ok=True)
        self.clean_opp(opp_dir)
        result = opentuner.MeasurementInterface(self.args).call_program(
            cmd=opp_compile_cmd, cwd=compile_dir)
        if result['returncode'] != 0:
            print("Failed to generate tuning opportunities, the error was:")
            print(result['stderr'])
            return None
        opp_files = glob.glob(os.path.join(opp_dir, "*"))
        if self.args.hot_func_file:
            hot_function_list = utils.parse_hot_function(
                self.args.hot_func_file, self.args.hot_func_number)
            self.args.func_name_filter = list(
                set(hot_function_list).union(set(self.args.func_name_filter)))
        search_space_tree = iomanager.generate_search_space(
            opp_files,
            self.args.search_config_file,
            type_filter=[phase],
            func_name_filter=self.args.func_name_filter,
            file_name_filter=self.args.file_name_filter)
        return search_space_tree

    def dump(self):
        output = self.args.output or 'output'
        get_remarks(self.args.config_file, output)
        get_results(self.args.database, 'OK', output)

    def add_func_instrument(self, compile_cmd, compile_dir):
        functions = []
        if os.path.exists(self.args.add_func_instrument):
            with open(self.args.add_func_instrument, 'r') as f:
                functions = [i.strip() for i in f.readlines() if i.strip()]
        output = os.path.join(compile_dir, 'instrument.out')
        if os.path.exists(output):
            os.remove(output)
        hook_file = add_func_instrument(compile_dir, functions, output=output)
        print("--> hook file:", hook_file)
        print("--> instrumentation file:", output)
        compile_cmd += ' ' + hook_file
        if '-rdynamic' not in compile_cmd:
            compile_cmd += ' -rdynamic'
        if ' -finstrument-functions' not in compile_cmd:
            compile_cmd += ' -finstrument-functions'
        if self.args.instrument_exclude_file_list:
            compile_cmd += ' ' + self.args.instrument_exclude_file_list.strip()
        return compile_cmd

    def create_func_instrument(self, compile_dir: str):
        functions = []
        if os.path.exists(self.args.create_func_instrument):
            with open(self.args.create_func_instrument, 'r') as f:
                functions = [i.strip() for i in f.readlines() if i.strip()]
        output = os.path.join(compile_dir, 'instrument.out')
        if os.path.exists(output):
            os.remove(output)
        hook_file = add_func_instrument(compile_dir, functions, output=output)
        print("--> hook file:", hook_file)
        print("===" * 20)
        print(
            f"$ gcc ${{src_files}} ${{options}} {hook_file} -rdynamic -finstrument-functions\n\n"
            f"# Use the following commands to exclude files or functions:\n"
            f"# -finstrument-functions-exclude-file-list=file,file,…\n"
            f"# -finstrument-functions-exclude-function-list=sym,sym,…")
        print("===" * 20)

    def generate_cl_options_config(self, config_file, compile_cmd,
                                   compile_dir):
        import shlex
        import hashlib
        import subprocess

        args = shlex.split(compile_cmd.replace('\\"', '&&'))
        valid_cmds = [
            arg.strip() for arg in args
            if arg.strip() and not arg.startswith('#')
        ]

        class CoarseTuningOptions:
            full_options = ['-funroll-loops']
            prefixes = ['-D', '-O']

            @classmethod
            def generate(cls, args):
                _, dyn_args = cls.split_options(cls, args)
                cls.config_data = [{
                    'Args': [{
                        'Order': i
                    }],
                    'CodeRegionHash':
                    int.from_bytes(hashlib.sha256(
                        f'{arg}-{i}'.encode()).digest(),
                                   byteorder='little'),
                    'CodeRegionType':
                    'coarse',
                    'DebugLoc': {},
                    'Function':
                    arg.replace(" ", "$"),
                    'Invocation':
                    0,
                    'Name':
                    arg.replace(" ", "$"),
                    'Pass':
                    arg.replace(" ", "$")
                } for i, arg in enumerate(dyn_args)]
                return cls()

            @classmethod
            def autotune(cls, args, config_file):
                _, dyn_args = cls.split_options(cls, args)
                src_config_data, config_data = [], []
                for i, arg in enumerate(dyn_args):
                    src_config_data.append(
                        int.from_bytes(hashlib.sha256(
                            f'{arg}-{i}'.encode()).digest(),
                                       byteorder='little'))
                for remark in get_remarks(config_file):
                    code_region_hash = remark.CodeRegionHash
                    if code_region_hash not in src_config_data:
                        print(code_region_hash)
                        logger.error(f"Invalid code region: {remark}")
                        exit(0)
                    config_data.append({
                        'Order':
                        remark.Args[0]['Order'],
                        'CodeRegionHash':
                        remark.CodeRegionHash,
                        'Name':
                        remark.Name.replace('$', ' ').replace('&&', '\"')
                    })
                if len(config_data) != len(src_config_data):
                    logger.error(f"Configuration number mismatch.")
                    exit(0)

                config_data = sorted(config_data, key=lambda x: x['Order'])
                cls.dyn_args = [i['Name'] for i in config_data]

                return cls()

            def split_options(self, args):
                self.static_args, self.dyn_args = [], []
                for arg in args:
                    is_dyn = False
                    for prefix in self.prefixes:
                        if arg.startswith(prefix):
                            is_dyn = True
                            break
                    for name in self.full_options:
                        if name == arg:
                            is_dyn = True
                            break
                    if is_dyn:
                        self.dyn_args.append(arg)
                    else:
                        self.static_args.append(arg)
                return self.static_args, self.dyn_args

            def dump(self, output):
                yaml_data = '!AutoTuning ' + '\n--- !AutoTuning '.join(
                    [str(i).replace("'", "") for i in self.config_data])
                with open(output, 'w', encoding='utf-8') as file:
                    file.write(yaml_data)

                return ' '.join(sys.argv)

            def run(self):
                print('===>', ' '.join(self.static_args + self.dyn_args))
                subprocess.check_call(self.static_args + self.dyn_args,
                                      cwd=compile_dir)

        if not os.path.exists(config_file):
            return CoarseTuningOptions.generate(valid_cmds).dump(config_file)
        else:
            CoarseTuningOptions.autotune(valid_cmds, config_file).run()
            exit(0)


def main():
    AutoTuner().run()


if __name__ == "__main__":
    main()
