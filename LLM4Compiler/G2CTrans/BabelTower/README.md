---
license: apache-2.0
size_categories:
- n<1K
---

# BabelTower Dataset

## Dataset Description
BabelTower is a paired corpora for translating C to CUDA, featuring 233 pairs of functionally aligned C and CUDA programs with tests. It can evaluate the ability of language models to convert sequential programs into parallel counterparts.

## Dataset Structure

```python
from datasets import load_dataset
load_dataset("kcxain/BabelTower")
DatasetDict({
  test: Dataset({
  features: ['id', 'cpp_code', 'cuda_code', 'consistent_cpp_inputs', 'consistent_cuda_inputs', 'cuda_wrapper', 'consistent_outputs'],
  num_rows: 233
  })
})
```

### How to use it

You can load and iterate through the dataset with the following two lines of code:

```python
from datasets import load_dataset

ds = load_dataset("kcxain/BabelTower", split="test")
sample = next(iter(ds))
print(sample)
# OUTPUT:
{
    'id': 0,
    'cpp_code': 'void add_sources_d ( const float * const model , float * wfp , const float * const source_amplitude , const int * const sources_z , const int * const sources_x , const int nz , const int nx , const int nt , const int ns , const int it ) { int x ; int b ; for ( x = 0 ; x < nx ; x ++ ) { for ( b = 0 ; b < ns ; b ++ ) { int i = sources_z [ b * ns + x ] * nx + sources_x [ b * ns + x ] ; int ib = b * nz * nx + i ; wfp [ ib ] += source_amplitude [ b * ns * nt + x * nt + it ] * model [ i ] ; } } }',
    'cuda_code': '__global__ void add_sources_d ( const float * const model , float * wfp , const float * const source_amplitude , const int * const sources_z , const int * const sources_x , const int nz , const int nx , const int nt , const int ns , const int it ) { int x = threadIdx . x ; int b = blockIdx . x ; int i = sources_z [ b * ns + x ] * nx + sources_x [ b * ns + x ] ; int ib = b * nz * nx + i ; wfp [ ib ] += source_amplitude [ b * ns * nt + x * nt + it ] * model [ i ] ; }',
    'consistent_cpp_inputs': ['float model4[] = {5.0}; float wfp4[] = {0.0}; float source_amplitude4[] = {3.0}; int sources_z4[] = {0}; int sources_x4[] = {0}; wrapper(add_sources_d, model4, wfp4, source_amplitude4, sources_z4, sources_x4, 1, 1, 1, 1, 0); '],
    'consistent_cuda_inputs': ['float model4[] = {5.0}; float wfp4[] = {0.0}; float source_amplitude4[] = {3.0}; int sources_z4[] = {0}; int sources_x4[] = {0}; wrapper(add_sources_d_cuda_invoke_in_cpp, model4, wfp4, source_amplitude4, sources_z4, sources_x4, 1, 1, 1, 1, 0); '],
    'cuda_wrapper': 'void add_sources_d_cuda_invoke_in_cpp(   const float* const model,  float* wfp,  const float* const source_amplitude,  const int* const sources_z,  const int* const sources_x,  const int nz,  const int nx,  const int nt,  const int ns,  const int it)  {   float* d_model;   float* d_wfp;   float* d_source_amplitude;   int* d_sources_z;   int* d_sources_x;  cudaMalloc((void**)&d_model, nz * nx * sizeof(float));   cudaMalloc((void**)&d_wfp, nz * nx * sizeof(float));   cudaMalloc((void**)&d_source_amplitude, ns * nt * sizeof(float));   cudaMalloc((void**)&d_sources_z, ns * sizeof(int));   cudaMalloc((void**)&d_sources_x, ns * sizeof(int));  cudaMemcpy(d_model, model, nz * nx * sizeof(float), cudaMemcpyHostToDevice);   cudaMemcpy(d_wfp, wfp, nz * nx * sizeof(float), cudaMemcpyHostToDevice);   cudaMemcpy(d_source_amplitude, source_amplitude, ns * nt * sizeof(float), cudaMemcpyHostToDevice);   cudaMemcpy(d_sources_z, sources_z, ns * sizeof(int), cudaMemcpyHostToDevice);   cudaMemcpy(d_sources_x, sources_x, ns * sizeof(int), cudaMemcpyHostToDevice);  add_sources_d<<<ns, nt>>>(d_model, d_wfp, d_source_amplitude, d_sources_z, d_sources_x, nz, nx, nt, ns, it);  cudaMemcpy(wfp, d_wfp, nz * nx * sizeof(float), cudaMemcpyDeviceToHost);  cudaFree(d_model);   cudaFree(d_wfp);   cudaFree(d_source_amplitude);   cudaFree(d_sources_z);   cudaFree(d_sources_x); }',
    'consistent_outputs': ['Return value: void Arguments after function call: ([ 5 ], [ 15 ], [ 3 ], [ 0 ], [ 0 ], 1, 1, 1, 1, 0) ']
}
```
For each data entry, the `consistent_cpp_inputs` and `consistent_cuda_inputs` fields contain executable test cases, while the `consistent_outputs` field provides the expected results for each test for both C and CUDA, including the function’s return value and the final values of all arguments (or the data pointed to by pointers) after the function call.

The C code in `cpp_code` can be executed directly. For CUDA code, the `cuda_code` should be combined with `cuda_wrapper`, and the function defined in the wrapper should be called to execute the kernel properly.

### Eval Pass@k for C-to-CUDA translation

We provide a script for evaluating various language models on the C-to-CUDA translation task in BabelTower using the Pass@k metric. The code is available at [MuSL](https://github.com/kcxain/musl).

### Data Fields

| **Field** | **Type** | **Description** |
|-----------|----------|-----------------|
| `id` | `int` | Unique identifier for each problem instance |
| `cpp_code` | `string` | Source C code implementing the sequential version |
| `cuda_code` | `string` | Reference CUDA implementation corresponding to the C code |
| `consistent_cpp_inputs` | `List[str]` | Executable test inputs for the C code |
| `consistent_cuda_inputs` | `List[str]` | Input cases equivalent to `consistent_cpp_inputs`, adapted for CUDA execution |
| `cuda_wrapper` | `string` | Host-side wrapper function for invoking the CUDA kernel in `cuda_code` |
| `consistent_outputs` | `List[str]` | Expected outputs from running the inputs on both C and CUDA code, including return values and post-execution argument states |


### Dataset Statistics
* 233 C-CUDA pairs
* 948 test cases
* all pairs have a least one test case

## Dataset Creation

The BabelTower corpus was constructed by crawling C and CUDA code from GitHub and manually aligning functionally equivalent pairs. For each pair, input test cases were generated using GPT-4, followed by filtering out incorrect cases. The remaining valid inputs were executed on both implementations to collect consistent output results. For more details please refer to the original [paper](https://arxiv.org/abs/2506.11153).

## Citation Information

```
@misc{ke2025mutualsupervisedlearningsequentialtoparallelcode,
      title={Mutual-Supervised Learning for Sequential-to-Parallel Code Translation}, 
      author={Changxin Ke and Rui Zhang and Shuo Wang and Li Ding and Guangli Li and Yuanbo Wen and Shuoming Zhang and Ruiyuan Xu and Jin Qin and Jiaming Guo and Chenxi Wang and Ling Li and Qi Guo and Yunji Chen},
      year={2025},
      eprint={2506.11153},
      archivePrefix={arXiv},
      primaryClass={cs.SE},
      url={https://arxiv.org/abs/2506.11153}, 
}

@InProceedings{pmlr-v162-wen22b,
      title = 	 {{B}abel{T}ower: Learning to Auto-parallelized Program Translation},
      author =       {Wen, Yuanbo and Guo, Qi and Fu, Qiang and Li, Xiaqing and Xu, Jianxing and Tang, Yanlin and Zhao, Yongwei and Hu, Xing and Du, Zidong and Li, Ling and Wang, Chao and Zhou, Xuehai and Chen, Yunji},
      booktitle = 	 {Proceedings of the 39th International Conference on Machine Learning},
      year = 	 {2022},
}
``` 