# AI4C

## 1 AI4C 介绍

AI4C 代表 AI 辅助编译器的套件，是一个使编译器能够集成机器学习驱动编译优化的框架。

## 2 软件架构说明

本框架包含以下几个模块，自动编译调优工具依赖 python 环境：

* AI辅助编译优化的推理引擎，驱动编译器在优化 pass 内使用AI模型推理所获得的结果实现编译优化。
  * 当前 GCC 内的 AI 使能优化 pass 基本通过编译器插件的形式实现，与编译器主版本解耦。
* 自动编译调优工具，通过编译器外部的调优工具（OpenTuner）驱动编译器执行多层粒度的自动编译调优，当前支持 GCC 和 LLVM 编译器。
  * 选项调优工具，用于应用级的编译选项调优。
  * 编译调优工具，基于 [Autotuner](https://gitee.com/openeuler/BiSheng-Autotuner) 实现，可实现细粒度和粗粒度的编译调优。
    * 细粒度调优，调优优化 pass 内的关键优化参数，例如，循环展开的次数（unroll count）。
    * 粗粒度调优，调优函数级的编译选项。

未来规划方向：

- [ ] 集成 [ACPO](https://gitee.com/src-openeuler/ACPO) 的 LLVM 编译优化模型，同时将 ACPO LLVM 侧的相关代码提取成插件，与 LLVM 主版本解耦。
- [ ] AI4Compiler 框架支持更多的开源机器学习框架的推理（pytorch - LibTorch、tensorflow - LiteRT）。
- [ ] 提供更多的 AI 辅助编译优化模型及相应的编译器插件。
- [ ] 集成新的搜索算法（基于白盒信息）并优化参数搜索空间（热点函数调优）。
- [ ] 支持 JDK 的编译参数调优。

## 3 AI4C 的安装构建

### 3.1 直接安装AI4C 

若用户使用最新的openEuler系统（24.03-LTS-SP1），同时只准备使用`AI4C`的现有特性，可以直接安装`AI4C`包。

```shell
yum install -y AI4C
```

若用户使用其他版本的`AI4C`特性或在其他OS版本中安装`AI4C`，需重新构建`AI4C`，可以参考以下步骤。

### 3.2 RPM包构建安装流程（推荐）

1. 使用 root 权限，安装 rpmbuild、rpmdevtools，具体命令如下：

   ```bash
   # 安装 rpmbuild
   yum install dnf-plugins-core rpm-build
   # 安装 rpmdevtools
   yum install rpmdevtools
   ```

2. 在主目录`/root`下生成 rpmbuild 文件夹：

   ```bash
   rpmdev-setuptree
   # 检查自动生成的目录结构
   ls ~/rpmbuild/
   BUILD  BUILDROOT  RPMS  SOURCES  SPECS  SRPMS
   ```

3. 使用`git clone https://gitee.com/src-openeuler/AI4C.git`，从目标仓库的 `openEuler-24.03-LTS-SP1` 分支拉取代码，并把目标文件放入 rpmbuild 的相应文件夹下：

   ``` shell
   cp AI4C/AI4C-v%{version}-alpha.tar.gz ~/rpmbuild/SOURCES/
   cp AI4C/*.patch ~/rpmbuild/SOURCES/
   cp AI4C/AI4C.spec ~/rpmbuild/SPECS/
   ```

4. 用户可通过以下步骤生成 `AI4C` 的 RPM 包：

   ```shell
   # 安装 AI4C 所需依赖
   yum-builddep ~/rpmbuild/SPECS/AI4C.spec
   # 构建 AI4C 依赖包
   rpmbuild -ba ~/rpmbuild/SPECS/AI4C.spec
   # 安装 RPM 包
   cd ~/rpmbuild/RPMS/
   rpm -ivh AI4C-<version>-<release>.<arch>.rpm
   ```

   注意事项：若系统因存有旧版本的 RPM 安装包而导致文件冲突，可以通过以下方式解决：

   ```shell
   # 解决方案一：强制安装新版本
   rpm -ivh AI4C-<version>-<release>.<arch>.rpm --force
   # 解决方案二：更新安装包
   rpm -Uvh AI4C-<version>-<release>.<arch>.rpm
   ```

   安装完成后，系统内会存在以下文件：

   * `/usr/bin/ai4c-*`: AI 使能的编译器以及自动调优工具的 wrapper
   * `/usr/lib64/libonnxruntime.so`: ONNX Runtime 的推理框架动态库
   * `/usr/lib64/AI4C/*.onnx`: AI 辅助编译优化模型（ONNX 格式）
   * `/usr/lib64/python<version>/site-packages/ai4c/lib/*.so`:
     * AI 辅助编译优化的推理引擎动态库
     * AI 辅助编译优化与编译调优的编译器插件动态库
   * `/usr/lib64/python<version>/site-packages/ai4c/autotuner/*`: 粗、细粒度调优工具的相关文件
   * `/usr/lib64/python<version>/site-packages/ai4c/optimizer/*`: AI 辅助编译优化的相关文件
   * `/usr/lib64/python<version>/site-packages/ai4c/option_tuner/*`: 应用级编译选项调优的相关文件

### 3.3 源码构建安装流程

#### 3.3.1 安装 ONNX Runtime 依赖

方案一：

在 GitHub 下载 1.16.3 版本，并解压相应架构的 tgz 文件，地址：https://github.com/microsoft/onnxruntime/releases/tag/v1.16.3

方案二：

保证以下 onnxruntime 的依赖包已安装：

```shell
yum install -y cmake make gcc gcc-c++ abseil-cpp-devel boost-devel bzip2 python3-devel python3-numpy python3-setuptools python3-pip
```

使用 cmake 安装 onnxruntime：

```shell
cd path/to/your/AI4C/third_party/onnxruntime
cmake \
    -DCMAKE_INSTALL_PREFIX=path/to/your/onnxruntime \
    -Donnxruntime_BUILD_SHARED_LIB=ON \
    -Donnxruntime_BUILD_UNIT_TESTS=ON \
    -Donnxruntime_INSTALL_UNIT_TESTS=OFF \
    -Donnxruntime_BUILD_BENCHMARKS=OFF \
    -Donnxruntime_USE_FULL_PROTOBUF=ON \
    -DPYTHON_VERSION=%{python3_version} \
    -Donnxruntime_ENABLE_CPUINFO=ON \
    -Donnxruntime_DISABLE_ABSEIL=ON \
    -Donnxruntime_USE_NEURAL_SPEED=OFF \
    -Donnxruntime_ENABLE_PYTHON=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -S cmake
make -j %{max_jobs} && make install
```

#### 3.3.2 安装 AI4C 的其他构建依赖

保证以下依赖包已安装：

```shell
yum install -y python3-wheel openssl openssl-devel yaml-cpp yaml-cpp-devel gcc-plugin-devel libstdc++-static
```

#### 3.3.3 构建 AI4C 框架

```shell
cd path/to/your/AI4C/python
python3 setup.py bdist_wheel                       \
    -Donnxruntime_ROOTDIR=path/to/your/onnxruntime \
    -DCMAKE_BUILD_TYPE=Release                     \
    -DCMAKE_CXX_COMPILER=path/to/your/g++          \
    -DCMAKE_C_COMPILER=path/to/your/gcc
pip3 install dist/ai4c-<version>-<python_version>-<python_version>-<os>_<arch>.whl --force-reinstall --no-deps
```

安装完成后，系统内会存在以下文件：

* `~/.local/bin/ai4c-*`: AI 使能的编译器以及自动调优工具的 wrapper
* `path/to/your/onnxruntime/lib64/libonnxruntime.so`: ONNX Runtime 的推理框架动态库
* `path/to/your/AI4C/models/*.onnx`: AI 辅助编译优化模型（ONNX 格式）
* `~/.local/python<version>/site-packages/ai4c/lib/*.so`:
  * AI 辅助编译优化的推理引擎动态库
  * AI 辅助编译优化与编译调优的编译器插件动态库
* `~/.local/python<version>/site-packages/ai4c/autotuner/*`: 粗、细粒度调优工具的相关文件
* `~/.local/python<version>/site-packages/ai4c/optimizer/*`: AI 辅助编译优化的相关文件
* `~/.local/python<version>/site-packages/ai4c/option_tuner/*`: 应用级编译选项调优的相关文件

### 3.4 FAQ

#### 3.4.1 构建 onnxruntime 时，发现系统内的 cmake 版本较低，导致 onnxruntime 无法构建。

解决方案：AI4C-v0.2.0-alpha 版本存有 cmake-3.28.5 版本的源码，可以从源码构建该版本的 cmake，用于构建 onnxruntime。

​                  cmake 构建方式参考：https://gitee.com/src-openeuler/AI4C/blob/openEuler-22.03-LTS-SP4/AI4C.spec

​                  AI4C-v0.2.0-alpha 地址：https://gitee.com/openeuler/AI4C/releases/tag/v0.2.0-alpha

## 4 使用流程

### 4.1 AI 辅助编译优化

当前的 AI 辅助编译优化模块，主要由三部分输入组成：

- ONNX 模型，训练后的辅助编译优化模型。
- 编译器插件（**当前仅支持 GCC 编译器**），用于运行 ONNX 模型推理并获取优化参数。
- AI4Compiler 框架，提供 ONNX 推理引擎和 GCC 优化编译命令。

用户事先根据开源机器学习框架训练一个 AI 模型，输出成 ONNX 格式。同时，针对该 AI 模型提供一个对应的编译器插件，插件内至少包含三个模块：

* 提取 AI 模型所需的编译器输入特征。
* 驱动推理引擎调用 AI 模型执行推理。
* 标注推理结果回编译器的数据结构。

在下述测试例中，仅需要在每次编译目标二进制的编译命令中，增加三个与插件相关的编译选项：插件路径、插件对应的 AI 模型路径、推理引擎路径，即可在编译时使能 AI 辅助编译优化模型。

```shell
# 若 onnxruntime 安装在非系统的文件夹下，注意设置环境变量
# export LD_LIBRARY_PATH=path/to/your/onnxruntime/lib64/:$LD_LIBRARY_PATH

gcc_compiler=path/to/your/gcc
infer_engine_path=$(ai4c-gcc --inference-engine）
model_path=path/to/your/model.onnx
plugin_path=path/to/your/<model_plugin>.so

$gcc_compiler test.c -O2 -o test                            \
    -fplugin=$plugin_path                                   \
    -fplugin-arg-<model_plugin>-model=$model_path           \
    -fplugin-arg-<model_plugin>-engine=$infer_engine_path
```

当前已支持的插件存在于`$(ai4c-gcc --inference-engine）`的同目录下，已支持的模型存在于`path/to/your/AI4C/models`下。

**注意事项：**

* 编译 AI 模型对应的编译器插件与编译目标优化应用的编译器需保证为同一个，否则会出现编译器版本不一致导致的编译报错。
* 当前 AI4C 仅支持在 GCC 编译器 cc1 阶段实现的 AI 辅助编译优化 pass 使用插件形式。

详细的编译器插件开发流程与使用流程可以参照 [AI 辅助编译优化手册](https://gitee.com/openeuler/AI4C/blob/master/python/docs/gcc-opt.md) 和 [测试例](https://gitee.com/openeuler/AI4C/tree/master/python/test/optimizer/block_correction) 进行。

下面我们举两个位于不同编译阶段的 AI 辅助编译优化模型的使用例。**循环展开与函数内联模型**位于`cc1`编译优化阶段，使用 GCC 插件形式实现 AI 模型适配与推理；**BOLT 采样基本块精度修正模型**位于`BOLT`链接后优化阶段，模型适配层位于 [LLVM-BOLT](https://gitee.com/src-openeuler/llvm-bolt) 仓库。

#### 4.1.1 循环展开与函数内联模型

循环展开与函数内联模型对应的编译优化选项如下：

| 选项名                                             | 说明                                                         |
| -------------------------------------------------- | ------------------------------------------------------------ |
| -fplugin                                           | 指定循环展开与函数内联插件的**绝对路径**（`-fplugin=/path/to/ipa_inline_unroll_plugin.so`）。 |
| -fplugin-arg-ipa_inline_unroll_plugin-engine       | 指定函数内联 ONNX 模型的推理引擎**绝对路径**（`-fplugin-arg-ipa_inline_unroll_plugin-inline_model=/path/to/inference_engine.so`），需要与`-fplugin`同时开启。`/path/to/inference_engine.so`的路径可通过`ai4c-gcc --inference-engine`获得。 |
| -fplugin-arg-ipa_inline_unroll_plugin-inline_model | 指定函数内联 ONNX 模型的**绝对路径**（`-fplugin-arg-ipa_inline_unroll_plugin-inline_model=/path/to/inline_model.onnx`），需要与`-fplugin`和`-fplugin-arg-ipa_inline_unroll_plugin-engine`同时开启。 |
| -fplugin-arg-ipa_inline_unroll_plugin-unroll_model | 指定循环展开 ONNX 模型的**绝对路径**（`-fplugin-arg-ipa_inline_unroll_plugin-unroll_model=/path/to/unroll_model.onnx`），需要与`-fplugin`和`-fplugin-arg-ipa_inline_unroll_plugin-engine`同时开启。 |

用户可同时启用一个 GCC 插件内的多个 AI 辅助编译优化模型，例如：

```shell
gxx_compiler=path/to/your/g++
infer_engine_path=$(ai4c-gcc --inference-engine）
inline_model_path=path/to/your/inline_model.onnx
unroll_model_path=path/to/your/unroll_model.onnx
plugin_path=path/to/your/ipa_inline_unroll_plugin.so

$gxx_compiler test.cc -O3 -o test -funroll-loops                           \
    -fplugin=$plugin_path                                                  \
    -fplugin-arg-ipa_inline_unroll_plugin-engine=$infer_engine_path        \
    -fplugin-arg-ipa_inline_unroll_plugin-inline_model=$inline_model_path  \
    -fplugin-arg-ipa_inline_unroll_plugin-unroll_model=$unroll_model_path
```

#### 4.1.2 BOLT 采样基本块精度修正模型

BOLT 采样的基本块精度修正模型对应的 BOLT 优化选项如下：

| 选项名              | 说明                                                         |
| ------------------- | ------------------------------------------------------------ |
| -block-correction   | 开启 AI 优化 CFG BB Count 选项，需要与 `-model-path` 选项同时开启以指定 ONNX 模型。 |
| -model-path         | 指定 ONNX 模型的**绝对路径**（`-model-path=/path/to/model.onnx`），需要与`-block-correction`同时开启。 |
| -annotate-threshold | 使用模型预测结果的置信度阈值，默认是 0.95。                  |

BOLT 内自定义的优化选项可以通过 GCC 的`-fbolt-option`调用使能，例如：

```shell
g++ -fbolt-use=<gcov_file> -fbolt-target=<bin_file> -fbolt-option=\"-block-correction -model-path=path/to/your/block_correction_model.onnx\"
```

### 4.2 细粒度调优

此处我们以 GCC 内**循环展开**优化 pass 的细粒度调优为例，展开调优工具的使用流程。

当前的细粒度调优模块，由两部分输入组成：

* 应用的调优配置文件（.ini）：处理应用的编译流程、执行流程。
* 搜参空间配置文件（YAML）：Autotuner 阶段配置的选项调优搜参空间，可替换默认搜参空间。

当前细粒度调优基于 [Autotuner](https://gitee.com/openeuler/BiSheng-Autotuner) 实现：

1. 在编译器的`generate`阶段，生成一组可调优的编译数据结构与可调优系数集合，保存在`opp/*.yaml`内。
2. 根据额外提供的编译搜参空间（`search_space.yaml`）与可调优数据结构，Autotuner 通过调优算法针对每个可调优数据结构生成下一组调优系数，保存在`input.yaml`中。
3. 在编译器的`autotune`阶段，根据`input.yaml`内数据结构的 hash 值，将调优系数标注到对应的数据结构里，完成调优。

在开启细粒度调优前，需安装以下依赖包：

```shell
yum install -y BiSheng-Autotuner bisheng-opentuner
```

**注意事项：**

* 当前默认支持程序运行时间作为性能值。

详细使用信息，请参考[细粒度调优使用手册](https://gitee.com/openeuler/AI4C/blob/master/python/docs/autotuner.md) 与该测试例：https://gitee.com/openeuler/AI4C/tree/master/python/test/autotuner/loop_unroll

LLVM 编译器的细粒度调优请参考 [Autotuner](https://gitee.com/openeuler/BiSheng-Autotuner) 仓库的使用教程。

### 4.3 函数级的粗粒度调优

当前的函数级粗粒度调优模块，由三部分输入组成：

* 应用的调优配置文件（.ini）：处理应用的编译流程、执行流程。
* 搜参空间配置文件（YAML）：Autotuner 阶段配置的选项调优搜参空间，可替换默认搜参空间。
* 编译选项全集文件（YAML）：预先设置的编译选项搜索空间全集，默认文件位于`path/to/your/python<version>/site-packages/ai4c/autotuner/yaml/coarse_options.yaml`。

当前函数级粗粒度调优基于 [Autotuner](https://gitee.com/openeuler/BiSheng-Autotuner) 实现，可以帮助各函数使用不同的编译选项组合执行编译优化，其调优原理细粒度调优与一致。由于各函数可调优的编译选项众多，可预先对选项空间做裁剪。

在开启函数级的粗粒度调优前，需安装以下依赖包：

```shell
yum install -y BiSheng-Autotuner bisheng-opentuner
```

**注意事项：**

* 当前默认支持程序运行时间作为性能值。
* 粗粒度调优暂不支持 dump 数据库内保存的历史数据。
* 当前的粗粒度调优支持与当前版本的 GCC 版本（12.3.1）配套使用，其他编译器版本会出现部分编译选项不支持的问题。可在`path/to/your/AI4C/aiframe/include/option_utils.h`中注释编译器未识别的编译选项。

详细使用信息，请参考该测试例：https://gitee.com/openeuler/AI4C/tree/master/python/test/autotuner/coarse_tuning

LLVM 编译器的粗粒度调优请参考 [Autotuner](https://gitee.com/openeuler/BiSheng-Autotuner) 仓库的使用教程。

### 4.4 应用级选项调优

当前的应用级选项调优模块，主要由三部分输入组成：

* 应用的编译与运行脚本（shell）：处理应用的编译流程（并将生成的下一组选项替换进编译脚本内）、执行流程、和性能数据采集流程。
* 编译选项与动态库选项的搜参空间配置文件（YAML）：配置选项调优的搜参空间，可配置开关选项（编译优化/动态库）、编译参数、枚举选项。
* 性能值的配置文件（YAML）：配置多个性能项的权重，与目标优化方向（最大/最小值），需与“性能数据采集流程”所获取的性能值数量、顺序对应。

应用级选项调优工具将不断收集应用的性能数据，更新性能模型，并生成一组模型预期收益较高的新编译选项组合。通过应用的编译与运行脚本将新的编译选项组合替换进编译脚本内，生成新的二进制文件并执行下一轮运行。反复调优，获取历史最优性能值。

在开启应用级选项调优前，需安装以下依赖包：

```shell
pip install xgboost scikit-learn
yum install -y time
```

默认的选项与性能值配置文件存在于以下路径：`path/to/your/python<version>/site-packages/ai4c/option_tuner/input/*.yaml`

* `options.yaml`：编译选项配置文件
* `options_lib.yaml`：动态库选项配置文件
* `config_measure.yaml`：性能值配置文件

用户可根据需要修改编译选项与动态库选项配置文件，相关关键词为：

* `required_*`：必选调优项，将一直保留在调优中
* `bool_*`：可选的编译优化开关选项
* `interval_*`: 可选的编译参数（值选项，数据区间）
* `enum_*`: 可选的编译参数（枚举选项）

用户可根据需要修改性能值配置文件，相关关键词为：

* `weight`: 性能值权重
* `optim`: 目标优化方向（最大/最小值）

调优完成后，历史与最佳调优数据将保留在`${parent_dir}/tuning/train.csv`和`${parent_dir}/tuning/result.txt`中。

详细使用信息，请参考该测试例：https://gitee.com/openeuler/AI4C/tree/master/python/test/option_tuner

## 5 参与贡献

1. Fork 本仓库
2. 新建 Feat_xxx 分支
3. 提交代码
4. 新建 Pull Request