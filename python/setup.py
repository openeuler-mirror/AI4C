#!/usr/bin/env python3
# coding=utf-8
from setuptools import setup
from setuptools import find_packages
import os
import sys
import shutil
import subprocess

project, version = 'ai4c', '1.0.4'
debug, proc_num = False, 16

argv, ext_args = [], []
for arg in sys.argv:
    if arg.startswith('-D'):
        ext_args.append(arg)
    elif arg.startswith('-j'):
        proc_num = int(arg[2:])
    else:
        argv.append(arg)
sys.argv = argv


def build_extension():
    src_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    build_dir = os.path.join(src_dir, 'python/build')
    ext_dir = os.path.join(src_dir, f'python/{project}/lib')

    os.makedirs(build_dir, exist_ok=True)
    os.makedirs(ext_dir, exist_ok=True)

    cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + ext_dir]

    cfg = 'RelWithDebInfo' if debug else 'Release'
    build_args = ['--config', cfg]

    cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
    build_args += ['--', f'-j{proc_num}']

    env = os.environ.copy()
    env['CXXFLAGS'] = f'{env.get("CXXFLAGS", "")}'
    subprocess.check_call(['cmake', src_dir] + cmake_args + ext_args,
                          cwd=build_dir,
                          env=env)
    subprocess.check_call(['cmake', '--build', '.'] + build_args,
                          cwd=build_dir)


build_extension()

setup(name=project,
      include_package_data=True,
      version=version,
      description='AI for Compiler',
      url='https://gitee.com/openeuler/AI4C',
      author='Huawei Technologies Co. Ltd.',
      packages=find_packages() +
      ['ai4c.lib', 'ai4c.autotuner.yaml', 'ai4c.option_tuner.input'],
      package_data={
          'ai4c.lib': ['*.so'],
          'ai4c.autotuner': ['*.cdef'],
          'ai4c.autotuner.yaml': ['*.yaml'],
          'ai4c.option_tuner.input': ['*.yaml'],
      },
      entry_points={
          'console_scripts': [
              f'{project}-tune = {project}.autotuner.tuner:main',
              f'{project}-autotune = {project}.autotuner.main:main',
              f'{project}-gcc = {project}.optimizer.main:gcc',
              f'{project}-g++ = {project}.optimizer.main:gcc_cxx',
              f'{project}-option-tune = {project}.option_tuner.mytuner:main',
          ],
      })

shutil.rmtree(f'{project}.egg-info')
