import os
import stat
import argparse
import yaml
from typing import List
import subprocess as sp
from autotuner.optrecord import Remark
from autotuner.optrecord import Loader

TUNING_OPTIONS = ['UnrollCount']


class AutoTuning(Remark):
    yaml_tag = '!AutoTuning'

    Args = {}
    DebugLoc = {}


def comfirm_security(file_path: str, raise_err: bool = True):
    if not os.path.exists(file_path):
        if raise_err:
            raise argparse.ArgumentTypeError(
                "File does not exist: {}".format(file_path))
        else:
            return print("File does not exist: {}".format(file_path))

    stat_info = os.stat(file_path)
    if bool(stat_info.st_mode & (stat.S_IWGRP | stat.S_IWOTH)):
        os.chmod(file_path, 0o644)
    return file_path


def add_config_db_arguments(parser: argparse.ArgumentParser):
    parser.add_argument("--use-hash-matching",
                        dest="use_hash_matching",
                        action="store_true",
                        help="Assign same configuration to the opportunities "
                        "which have same hash value.")


def add_arg_search_space(parser: argparse.ArgumentParser):
    parser.add_argument("--search-space",
                        type=comfirm_security,
                        help="Check search space secure")


def add_arg_trials(parser: argparse.ArgumentParser, max_parallel: int = 4096):

    def positive_int(value):
        ivalue = int(value)
        if ivalue <= 0:
            raise argparse.ArgumentTypeError(
                "{} is an invalid positive int value".format(ivalue))
        if ivalue > max_parallel:
            raise argparse.ArgumentTypeError(
                "Maximum number of trials is {}".format(max_parallel))
        return ivalue

    parser.add_argument("--trials",
                        type=positive_int,
                        default=1,
                        help="Specify the number of trials to be tested "
                        "in the next iteration")
    return parser


def add_arg_deterministic(parser: argparse.ArgumentParser):

    def str2bool(value):
        if value.lower() in ['true', '1', 'yes', 't', 'y']:
            return True
        elif value.lower() in ['false', '0', 'no', 'f', 'n']:
            return False
        else:
            raise argparse.ArgumentTypeError("Invalid value: {}".format(value))

    parser.add_argument("--deterministic",
                        type=str2bool,
                        default=False,
                        help="Enable deterministic tuning mode to generate "
                        "reproducible results/output. For testing "
                        "purposes only; off by default. [True/False]")

    parser.add_argument("--seed",
                        default=0x31337,
                        help="Specifying the seed value for Random Number "
                        "Generator. For testing purposes only")

    parser.add_argument("--seed-file",
                        type=str,
                        help="Specify the path of seed file for Random Number "
                        "Generator. This option requires "
                        "'--deterministic=True'.")

    parser.add_argument("--use-optimal-configs",
                        dest="use_optimal_configs",
                        choices=["none", "reuse", "retune"],
                        default="none",
                        help="Use previously found/stored configurations for "
                        "code regions for current tuning.\n"
                        "Options: {none[Default], reuse, retune}. "
                        "reuse/retune can only be used when "
                        "'use-hash-matching' is enabled.\n"
                        "none: Do not reuse the old configurations.\n"
                        "reuse: Reuse the old optimal configurations "
                        "found and tune code regions which don't have "
                        "optimal configurations stored in database.\n"
                        "retune: Retune all the code regions and use "
                        "the optimal configurations (found in database) "
                        "as starting point for AutoTuner.\n")


def add_code_region_filtering_arguments(parser: argparse.ArgumentParser):
    parser.add_argument('--name-filter',
                        nargs='+',
                        metavar='Name',
                        default=[],
                        help='Generate search space to include only code '
                        'regions named in space-delimited list.')
    parser.add_argument('--func-name-filter',
                        nargs='+',
                        metavar='Name',
                        default=[],
                        help='Generate search space to include only code '
                        'regions having function name in space-delimited '
                        'list.')
    parser.add_argument('--file-name-filter',
                        nargs='+',
                        metavar='Name',
                        default=[],
                        help='Generate search space to include only code '
                        'regions having file name in space-delimited '
                        'list.')
    parser.add_argument('--pass-filter',
                        nargs='+',
                        metavar='Name',
                        default=[],
                        help='Generate search space to include only code '
                        'regions of a specific pass.')
    parser.add_argument('--type-filter',
                        nargs='+',
                        metavar='Name',
                        default=[],
                        help='Generate search space to include only code '
                        'regions having type in space-delimited list.\n'
                        'Options: [loop, callsite, machine_basic_block, '
                        'other, llvm-param, program-param',
                        choices=[
                            'loop', 'callsite', 'machine_basic_block', 'other',
                            'llvm-param', 'program-param'
                        ])


def add_use_dynamic_values(parser: argparse.ArgumentParser):
    parser.add_argument('--use-dynamic-values',
                        action='store_true',
                        help='Turn on dynamic values suggested by the compiler'
                        'Default: turned off')


def add_arg_baseline_config(parser: argparse.ArgumentParser):
    parser.add_argument("-b",
                        '--use-baseline-config',
                        action='store_true',
                        help='Start the search from the baseline configuration'
                        ' instead of a random point in the search space'
                        ' (default).')


def get_remarks(input_file: str, output: str = None):
    all_remarks: List[AutoTuning] = []

    with open(input_file, 'r') as input_file_handler:
        docs = yaml.load_all(input_file_handler, Loader=Loader)
        for remark in docs:
            all_remarks.append(remark)

    if not output:
        return all_remarks

    loop_config, func_config = [], []
    for remark in all_remarks:
        loop_config.append(f'{remark.Pass},{remark.Name},{remark.File},'
                           f'{remark.Function},{remark.Line},{remark.Column}')
        func_config.append(remark.Function)

    output_loop_config = output + '_config.csv'
    with open(output_loop_config, 'w') as f:
        print('Pass,Name,File,Function,Line,Column', file=f)
        print('\n'.join(loop_config), file=f)
    print('>>', output_loop_config)

    output_func_config = output + '_func_instrument'
    with open(output_func_config, 'w') as f:
        print('\n'.join(list(set(func_config))), file=f)
    print('>>', output_func_config)


def get_results(input_db: str, state: str = None, output: str = None):
    import sqlite3
    import pickle
    from gzip import zlib

    output_file = output + '_data.csv'
    with sqlite3.connect(input_db) as conn:
        cursor = conn.cursor()

        state_cond = f"where r.state = '{state}'" if state else ''
        cursor.execute(f"""select r.id, r.configuration_id,
                                    r.collection_cost, r.time,
                                    r.state, r.tuning_run_id,
                                    c.hash, c."data"
                            from "result" r
                            join configuration c on r.configuration_id = c.id
                            {state_cond};""")

        desc = [i[0] for i in cursor.description]
        rows = cursor.fetchall()

        file = open(output_file, 'w')
        print(','.join(desc[:-1] + TUNING_OPTIONS), file=file)

        for row in rows:
            row = list(row)
            data, row = row[-1], row[:-1]
            try:
                data: dict = pickle.loads(zlib.decompress(data))
                for option in TUNING_OPTIONS:
                    config = []
                    for k, v in data.items():
                        if option in k:
                            config.append(v)
                    row.append('"' + str(config).replace(' ', '') + '"')
            except Exception:
                pass
            print(','.join([str(i) for i in row]), file=file)

        cursor.close()
        file.close()
    print('>>', output_file)


with open(os.path.join(os.path.dirname(__file__), "hook.cdef"), "r") as hdf:
    hook_cdef = hdf.read()
with open(os.path.join(os.path.dirname(__file__), "hook_all_func.cdef"),
          "r") as hdf:
    hook_all_cdef = hdf.read()


def add_func_instrument(save_dir: str,
                        func_list: list,
                        output: str = "instrument.out"):
    hook_dir = os.path.join(save_dir, ".instrument")
    os.makedirs(hook_dir, exist_ok=True)
    hook_file = os.path.join(hook_dir, "hook.cpp")
    unified_func_list = [f'"{func}"' for func in func_list]
    func_num = len(unified_func_list)
    with open(hook_file, "w") as f:
        if func_num == 0:
            print("\033[93m\033[1mWARN: print all functions may result in"
                  " a waste of performance and storage space. \033[0m")
            new_hook_cdef = hook_all_cdef
            new_hook_cdef = new_hook_cdef.replace("${output}", output)
        else:
            new_hook_cdef = hook_cdef
            new_hook_cdef = new_hook_cdef.replace("${func_num}", str(func_num))
            new_hook_cdef = new_hook_cdef.replace(
                "${functions}", ",\n    ".join(unified_func_list))
            new_hook_cdef = new_hook_cdef.replace("${output}", output)
        f.write(new_hook_cdef)

    sp.check_call(['g++', 'hook.cpp', '-c', '-o', 'hook.o'], cwd=hook_dir)

    return os.path.join(hook_dir, "hook.o")
