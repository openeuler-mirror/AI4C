#ifndef BOLT_PROFILE_ONNXRUNNER_H
#define BOLT_PROFILE_ONNXRUNNER_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include "onnxruntime_c_api.h"
#include "onnxruntime_cxx_api.h"

extern "C" {
namespace boltONNXRunner {
class ONNXRunner {
public:
    explicit ONNXRunner() {}
    explicit ONNXRunner(const char* model_path) {
        // prepare model and env
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);
        session = new Ort::Session(env, model_path, session_options);
    }
    ~ONNXRunner() {
        delete session;
    }
    float runONNXModel(std::vector<std::string> input_string, std::vector<int64_t> input_int64, std::vector<float> input_float);

private:
    static Ort::Value getInputValueFloat(Ort::Session *session, 
                                  std::vector<float> &input,
                                  int inputIdx);

    static Ort::Value getInputValueString(Ort::AllocatorWithDefaultOptions allocator,
                                   Ort::Session *session, 
                                   std::vector<std::string> &input,
                                   int inputIdx);
    
    static Ort::Value getInputValueInt64(Ort::Session *session, 
                                  std::vector<int64_t> &input,
                                  int inputIdx);
    
    Ort::SessionOptions session_options;
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "test"};
    Ort::Session *session;
};

extern ONNXRunner* createONNXRunner(const char* model_path) {
    return new ONNXRunner(model_path);
}

extern void deleteONNXRunner(ONNXRunner* instance) {
    if (instance != nullptr) {
        delete instance;
    }
}

extern float runONNXModel(ONNXRunner* instance, std::vector<std::string> input_string, std::vector<int64_t> input_int64, std::vector<float> input_float) {
    if (instance != nullptr) {
        return instance->runONNXModel(input_string, input_int64, input_float);
    } else { 
        return -1;
    }
}
} // namespace boltONNXRunner
} // extern "C"
#endif
