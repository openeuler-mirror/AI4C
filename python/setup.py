#!/usr/bin/env python3
# coding=utf-8
from setuptools import setup, Extension
from setuptools import find_packages
from setuptools.command.build_ext import build_ext
import os
import sys
import shutil
import subprocess

project, version = 'ai4c', '1.0.3'

argv, ext_args = [], []
for arg in sys.argv:
    if arg.startswith('-D'):
        ext_args.append(arg)
    else:
        argv.append(arg)
sys.argv = argv


class CMakeExtension(Extension):

    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):

    def run(self):
        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext: CMakeExtension):
        extdir = os.path.abspath(
            os.path.dirname(self.get_ext_fullpath(ext.name)))
        extdir = os.path.join(extdir, f'{project}/lib')
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir]

        cfg = 'RelWithDebInfo' if self.debug else 'Release'
        build_args = ['--config', cfg]

        cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
        build_args += ['--', '-j16']

        env = os.environ.copy()
        env['CXXFLAGS'] = f'{env.get("CXXFLAGS", "")} -DVERSION_INFO=' \
                          f'\\"{self.distribution.get_version()}\\"'
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args + ext_args,
                              cwd=self.build_temp,
                              env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args,
                              cwd=self.build_temp)


setup(name=project,
      include_package_data=True,
      version=version,
      description='AI for Compiler',
      url='https://gitee.com/openeuler/AI4C',
      author='Huawei Technologies Co. Ltd.',
      packages=find_packages() +
      ['ai4c.autotuner.yaml', 'ai4c.option_tuner.input'],
      package_data={
          'ai4c.autotuner': ['*.cdef'],
          'ai4c.autotuner.yaml': ['*.yaml'],
          'ai4c.option_tuner.input': ['*.yaml']
      },
      ext_modules=[CMakeExtension(project, '..')],
      cmdclass={'build_ext': CMakeBuild},
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
