#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
import glob
from typing import List
from argparse import ArgumentParser, Namespace
import subprocess as sp
import logging

logger = logging.getLogger(__name__)


def run_command(command):
    return sp.run(command,
                  shell=True,
                  stdout=sp.PIPE,
                  stderr=sp.PIPE,
                  text=True)


class AIEnabledCompile:

    def __init__(self,
                 args: Namespace,
                 options: List[str],
                 executor: str = 'gcc') -> None:
        self.lib_path = os.path.abspath(
            os.path.join(os.path.dirname(__file__), "..", "lib"))

        self.info(args)
        self.prepare(args, options, executor)

    def prepare(self, args: Namespace, options: List[str], executor: str):
        add_opt_plugin = args.add_opt_plugin or []
        add_opt_model = args.add_opt_model or []
        if len(add_opt_plugin) != len(add_opt_model):
            raise ValueError('the number of opt-plugins'
                             f'[{len(add_opt_plugin)}] and opt-models'
                             f'[{len(add_opt_model)}] are mismatch')

        self.gcc_executor = executor
        self.gcc_options = [self.gcc_executor, *options]
        infer_lib = os.path.join(self.lib_path, 'libONNXRunner.so')
        for plugin, model in zip(add_opt_plugin, add_opt_model):
            if not os.path.exists(plugin):
                plugin_ = os.path.join(self.lib_path, plugin)
                if not os.path.exists(plugin_):
                    raise FileNotFoundError(f'Plugin not found: {plugin}')
                plugin = plugin_
            if not os.path.exists(model):
                default_model_path = os.path.abspath(
                    os.path.join(os.path.dirname(__file__), "..", "ai_models"))
                model_ = os.path.join(default_model_path, model)
                if not os.path.exists(model_):
                    raise FileNotFoundError(f'Model not found: {model}')
                model = model_

            self.gcc_options.append(f'-fplugin={plugin}')
            plugin_name = plugin.split('/')[-1][:-3]
            self.gcc_options.append(
                f'-fplugin-arg-{plugin_name}-model={model}')
            self.gcc_options.append(
                f'-fplugin-arg-{plugin_name}-infer={infer_lib}')

    def info(self, args: Namespace):
        if args.list_plugins:
            plugins = glob.glob(f'{self.lib_path}/*plugin_gcc*.so')
            print('===' * 30 + '\n')
            print('* ' + '\n* '.join(plugins))
            print('\n' + '===' * 30)
            exit(0)
        if args.inference_engine:
            engine = glob.glob(f'{self.lib_path}/ai4c_onnxrunner.so')
            print(f'{engine[0]}', end='\n')
            exit(0)
        if args.yaml_tuning_config:
            yaml_file = glob.glob(f'{self.lib_path}/../autotuner/yaml/*.yaml')
            print(f'{yaml_file[0]}', end='\n')
            exit(0)

    def compile(self):
        print('\n$ ' + ' '.join(self.gcc_options) + '\n')
        sp.check_call(self.gcc_options)


def get_args() -> Namespace:
    parser = ArgumentParser(description='AI enabled optimized compilation')
    parser.add_argument('--list-plugins',
                        action='store_true',
                        help='list available plugins')
    parser.add_argument('--inference-engine',
                        action='store_true',
                        help='get inferenc engine (ai4c_onnxrunner.so) path')
    parser.add_argument('--yaml-tuning-config',
                        action='store_true',
                        help='get autotuner yaml config file path')
    parser.add_argument(
        '--add-opt-plugin',
        action='append',
        help='add a gcc-plugin for model enabled compiling optimization')
    parser.add_argument(
        '--add-opt-model',
        action='append',
        help='add a model (*.onnx) for the previous opt-plugin')
    args, other_options = parser.parse_known_args()
    return args, other_options


def gcc():
    executor = os.environ.get('AI4C_C_COMPILER', 'gcc')
    AIEnabledCompile(*get_args(), executor).compile()


def gcc_cxx():
    executor = os.environ.get('AI4C_CXX_COMPILER', 'g++')
    AIEnabledCompile(*get_args(), executor).compile()


if __name__ == "__main__":
    gcc()
