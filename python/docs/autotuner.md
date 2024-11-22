## GCC Autotuner

Using AI4Compiler framework to automatically optimize GCC compilation.

### GCC auto-tuning

In this case, we will take advantage of `CoreMark` as an example for applying our autotuning tools.
#### Section 1. Create a Tuning Configuration File

coremark_sample.ini
```ini
[DEFAULT] # optional
# PluginPath = /path/to/gcc-plugins

[Environment Setting]  # optional
# prepend a list of paths into the PATH in order.
# PATH = /path/to/bin
# you can also set other enviroment variables here too

[Compiling Setting] # required
# NOTE: ConfigFilePath is set to the path to the current config file automatically by default.
CompileDir = /path/to/coremark
LLVMInputFile = %(CompileDir)s/input.yaml

# OppDir and OppCompileCommand are optional, 
# do not have to specify this if not using auto_run sub-command
OppDir = autotune_datadir/opp

CompilerCXX = /path/to/bin/gcc
BaseCommand = %(CompilerCXX)s -I. -I./posix -DFLAGS_STR=\""  -lrt"\" \
                -DPERFORMANCE_RUN=1 -DITERATIONS=10000 -g            \
                core_list_join.c  core_main.c core_matrix.c          \
                core_state.c core_util.c posix/core_portme.c         \
                -funroll-loops -O2 -o coremark                       \
                -fplugin=%(PluginPath)s/rtl_unroll_autotune_plugin_gcc12.so

# auto-tuning
CompileCommand = %(BaseCommand)s \
    -fplugin-arg-rtl_unroll_autotune_plugin_gcc12-autotune=%(LLVMInputFile)s

RunDir = %(CompileDir)s
RunCommand = ./coremark 0x0 0x0 0x66 100000 # run 300000 iterations for coremark

# generate
OppCompileCommand = %(BaseCommand)s \
    -fplugin-arg-rtl_unroll_autotune_plugin_gcc12-generate=%(OppDir)s
```

#### Section 2. Create Tuning Search Space Profiles

search_space.yaml
```yaml
CodeRegion:
   CodeRegionType: loop
   Pass: loop2_unroll
   Args:
     UnrollCount:
       Value: [0, 1, 2, 4, 8, 16, 32]
       Type: enum
---
CodeRegion:
  CodeRegionType: callsite
  Pass: inline
  Args:
    ForceInline:
      Type: bool
```

#### Section 3. Run Autotuner

Place `coremark`, `coremark_sample.ini` and `search_space.yaml` 
in the same directory, and run them in the directory.
```sh
ai4c-autotune autorun coremark_sample.ini \
    -scf search_space.yaml --stage-order loop \
    --time-after-convergence=100
```
Where `--time-after-convergence` specifies the waiting time after the optimal 
configuration is generated. For example, if no better parameter configuration 
is generated for 100s after the optimal configuration is generated, automatic 
optimization stops.

#### Section 4. Export Tuning Data

```sh
ai4c-autotune dump -c coremark/input.yaml \
    --database=opentuner.db/localhost.localdomain.db -o autotune
```
which will export two csv files from database:
* `autotune_config.csv`
* `autotune_data.csv`


### GCC step-by-step Tuning

#### Section 1. Create a temporary storage path

```sh
cd /path/to/coremark
mkdir -p autotune_datadir/opp
```

#### Section 2. Generate opportunites with the gcc-plugin

```sh
# list all available gcc-plugins provided by ai4c-framework
ai4c-tune list

# replace "/path/to/rtl_unroll_autotune_plugin_gcc12.so" with your plugin
gcc -I. -I./posix -DFLAGS_STR=\""  -lrt"\" -DPERFORMANCE_RUN=1          \
    -DITERATIONS=10000 -g core_list_join.c core_main.c                  \
    core_matrix.c core_state.c core_util.c posix/core_portme.c          \
    -funroll-loops -fplugin=/path/to/rtl_unroll_autotune_plugin_gcc12.so         \
    -fplugin-arg-rtl_unroll_autotune_plugin_gcc12-generate=autotune_datadir/opp  \
    -O2 -o coremark 
```

#### Section 3. Generate tuning configuration based on search space and opportunities

```sh
ai4c-tune minimize --search-space=search_space.yaml --trials=1
```

#### Section 4. Compile with configuration and gcc-plugin
```sh
gcc -I. -I./posix -DFLAGS_STR=\""  -lrt"\" -DPERFORMANCE_RUN=1                 \
    -DITERATIONS=10000 -g core_list_join.c core_main.c                         \
    core_matrix.c core_state.c core_util.c posix/core_portme.c                 \
    -funroll-loops -fplugin=/path/to/rtl_unroll_autotune_plugin_gcc12.so                \
    -fplugin-arg-rtl_unroll_autotune_plugin_gcc12-autotune=autotune_datadir/config.yaml \
    -O2 -o coremark 
```

#### Section 5. Feedback program running time
```sh
time -p ./coremark  0x0 0x0 0x66 300000  2>&1 1>/dev/null | grep real | awk '{print $2}'

ai4c-tune feedback 16
```

#### Section 6. Repeat steps 4 and 5

Until the optimal program running time is obtained, terminate the optimization, 
and use the optimal configuration to perform the last compilation.

```sh
ai4c-tune finalize
```
