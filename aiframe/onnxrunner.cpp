#include "onnxrunner.h"

namespace ai4c {

ONNXRunner::ONNXRunner(const char* model_path) { load_model(model_path); }

ONNXRunner::~ONNXRunner() {
  if (session) {
    delete session;
    session = nullptr;
  }
}

void ONNXRunner::load_model(const char* model_path) {
  session_options.SetGraphOptimizationLevel(
      GraphOptimizationLevel::ORT_ENABLE_BASIC);
  session = new Ort::Session(env, model_path, session_options);
}

int ONNXRunner::inference() {
  int output_count = session->GetOutputCount();
  if (output_count == 0) {
    fprintf(stderr, "Model output is empty.\n");
    return -1;
  }
  for (int i = 0; i < output_count; i++) {
    std::string output = session->GetOutputNameAllocated(i, allocator).get();
    output_names_.push_back(output);
  }

  std::vector<const char*> input_names;
  for (std::string& name : input_names_) input_names.push_back(name.c_str());
  std::vector<const char*> output_names;
  for (std::string& name : output_names_) output_names.push_back(name.c_str());

  output_values_ = session->Run(Ort::RunOptions{nullptr}, input_names.data(),
                                input_values_.data(), input_names.size(),
                                output_names.data(), output_names.size());
  return 0;
}

#define ADD_INPUT_FEATURE(data_type)                                           \
  int i = input_values_.size();                                                \
  std::string input_name = session->GetInputNameAllocated(i, allocator).get(); \
  input_names_.push_back(input_name);                                          \
  auto type_info = session->GetInputTypeInfo(i);                               \
  auto shape_info = type_info.GetTensorTypeAndShapeInfo();                     \
  std::vector<int64_t> shape = shape_info.GetShape();                          \
  int elements =                                                               \
      std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int>());  \
  if (elements < 0) {                                                          \
    elements = -elements;                                                      \
    if (num % elements != 0) {                                                 \
      fprintf(stderr,                                                          \
              "input '%s' has invalid data size:"                              \
              " %d [%d]\n",                                                    \
              input_name.c_str(), num, elements);                              \
      return -1;                                                               \
    }                                                                          \
    shape[0] = num / elements;                                                 \
  } else if (elements != num) {                                                \
    fprintf(stderr, "input '%s' data size mismatch\n", input_name.c_str());    \
    return -1;                                                                 \
  }                                                                            \
  Ort::MemoryInfo memory_info =                                                \
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);        \
  input_values_.push_back(Ort::Value::CreateTensor<data_type>(                 \
      memory_info, data, num, shape.data(), shape.size()));                    \
  return 0;

int ONNXRunner::add_int64_input(int64_t* data, int num) {
  ADD_INPUT_FEATURE(int64_t)
}
int ONNXRunner::add_int32_input(int32_t* data, int num) {
  ADD_INPUT_FEATURE(int32_t)
}
int ONNXRunner::add_float_input(float* data, int num) {
  ADD_INPUT_FEATURE(float)
}
int ONNXRunner::add_double_input(double* data, int num) {
  ADD_INPUT_FEATURE(double)
}
int ONNXRunner::add_string_input(char** data, int num) {
  int index = input_values_.size();
  std::string input_name =
      session->GetInputNameAllocated(index, allocator).get();
  input_names_.push_back(input_name);
  auto type_info = session->GetInputTypeInfo(index);
  auto shape_info = type_info.GetTensorTypeAndShapeInfo();
  auto shape = shape_info.GetShape();
  auto memory_info =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

  int elements =
      std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int>());
  if (elements < 0) {
    elements = -elements;
    if (num % elements != 0) {
      fprintf(stderr,
              "input '%s' has invalid data size:"
              " %d [%d]\n",
              input_name.c_str(), num, elements);
      return -1;
    }
    shape[0] = num / elements;
  } else if (elements != num) {
    fprintf(stderr, "input '%s' data size mismatch\n", input_name.c_str());
    return -1;
  }

  input_values_.push_back(
      Ort::Value::CreateTensor(allocator, shape.data(), shape.size(),
                               ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING));
  input_values_[index].FillStringTensor(data, num);
  return 0;
}

int32_t* ONNXRunner::get_int32_output(int index) {
  if (index >= output_values_.size()) {
    return nullptr;
  }
  return output_values_[index].GetTensorMutableData<int32_t>();
}
int64_t* ONNXRunner::get_int64_output(int index) {
  if (index >= output_values_.size()) {
    return nullptr;
  }
  return output_values_[index].GetTensorMutableData<int64_t>();
}
float* ONNXRunner::get_float_output(int index) {
  if (index >= output_values_.size()) {
    return nullptr;
  }
  return output_values_[index].GetTensorMutableData<float>();
}

void ONNXRunner::clear() {
  input_values_.clear();
  input_names_.clear();
  output_names_.clear();
}

std::vector<float> ONNXRunner::get_probability(int batch_size) {
  std::vector<float> probs;
  for (int i = 0; i < batch_size; ++i) {
	  Ort::Value mapOut =
	       output_values_[1].GetValue(static_cast<int>(i), allocator);
	  Ort::Value keysOrt = mapOut.GetValue(0, allocator);
	  int64_t * keysRet = keysOrt.GetTensorMutableData<int64_t>();
	  Ort::Value valuesOrt = mapOut.GetValue(1, allocator);
	  float *valuesRet = valuesOrt.GetTensorMutableData<float>();
	  probs.push_back((*(valuesRet + 1)));
  } 
  return probs;
}

extern "C" {

static ONNXRunner* onnx_runner;

void initialize(const char* model_path) {
  if (!onnx_runner) {
    onnx_runner = new ONNXRunner(model_path);
  } else {
    onnx_runner->clear();
  }
}

int add_int64_input(int64_t* input, int num) {
  return onnx_runner->add_int64_input(input, num);
}

void add_int32_input(int32_t* input, int num) {
  onnx_runner->add_int32_input(input, num);
}

void add_float_input(float* input, int num) {
  onnx_runner->add_float_input(input, num);
}

void add_double_input(double* input, int num) {
  onnx_runner->add_double_input(input, num);
}

void add_string_input(char** input, int num) {
  onnx_runner->add_string_input(input, num);
}

int inference() { return onnx_runner->inference(); }

int32_t* get_int32_output(int index) {
  return onnx_runner->get_int32_output(index);
}
int64_t* get_int64_output(int index) {
  return onnx_runner->get_int64_output(index);
}
float* get_float_output(int index) {
  return onnx_runner->get_float_output(index);
}

void clear_engine() { onnx_runner->clear(); }
std::vector<float> get_probability(int batch_size) {
  return onnx_runner->get_probability(batch_size);
}
void free_engine() {
  if (onnx_runner) {
    delete onnx_runner;
    onnx_runner = NULL;
  }
}

}  // extern "C"
}  // namespace ai4c
