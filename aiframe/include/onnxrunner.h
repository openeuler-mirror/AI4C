#ifndef AI4C_ONNX_RUNNER_H
#define AI4C_ONNX_RUNNER_H
#include <onnxruntime_c_api.h>
#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace ai4c {
class ONNXRunner {
 public:
  ONNXRunner() {}
  ONNXRunner(const char* model_path);
  ~ONNXRunner();

  int inference();

  void loadModel(const char* model_path);

  int add_int64_input(int64_t* data, int num);
  int add_int32_input(int32_t* data, int num);
  int add_float_input(float* data, int num);
  int add_double_input(double* data, int num);
  int add_string_input(char** data, int num);

  int32_t* get_int32_output(int index);
  int64_t* get_int64_output(int index);
  float* get_float_output(int index);

  void clear();

 private:
  Ort::SessionOptions session_options;
  Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "ai4c"};
  Ort::AllocatorWithDefaultOptions allocator;
  Ort::Session* session;

  std::vector<std::string> input_names_;
  std::vector<Ort::Value> input_values_;
  std::vector<std::string> output_names_;
  std::vector<Ort::Value> output_values_;
};

}  // namespace ai4c
#endif  // AI4C_ONNX_RUNNER_H