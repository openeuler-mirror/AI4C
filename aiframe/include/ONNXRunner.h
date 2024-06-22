#ifndef COMPILER_PROFILE_ONNXRUNNER_H
#define COMPILER_PROFILE_ONNXRUNNER_H

#include "onnxruntime_c_api.h"
#include "onnxruntime_cxx_api.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <vector>

extern "C" {
namespace compilerONNXRunner {

const char* MODEL_PATH_OPT = "/usr/lib64/AI4C/optimizer.onnx";
const int FEATURE_SIZE_INT64_OPT = 6;
const int FEATURE_SIZE_STRING_OPT = 11;

class ONNXRunner {
public:
  explicit ONNXRunner() {}
  explicit ONNXRunner(const char *modelPath) {
    // Prepare model and env
    sessionOptions.SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_ENABLE_BASIC);
    session = new Ort::Session(env, modelPath, sessionOptions);
  }
  ~ONNXRunner() { delete session; }
  std::vector<float> runONNXModel(std::vector<std::string> inputString,
                                  std::vector<int64_t> inputInt64,
                                  std::vector<float> inputFloat, int batchSize);
  int64_t runONNXModelOptimizer(std::vector<std::string> inputString,
                                std::vector<int64_t> inputInt64,
                                std::vector<float> inputFloat, int batchSize);

private:
  static Ort::Value getInputValueFloat(Ort::Session *session,
                                       std::vector<float> &input, int inputIdx,
                                       int batchSize);

  static Ort::Value
  getInputValueString(Ort::AllocatorWithDefaultOptions allocator,
                      Ort::Session *session, std::vector<std::string> &input,
                      int inputIdx, int batchSize);

  static Ort::Value getInputValueInt64(Ort::Session *session,
                                       std::vector<int64_t> &input,
                                       int inputIdx, int batchSize);

  Ort::SessionOptions sessionOptions;
  Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "test"};
  Ort::Session *session;
};

extern ONNXRunner *createONNXRunner(const char *modelPath) {
  std::ifstream file(modelPath);
  if (file.good()) {
    return new ONNXRunner(modelPath);
  } else {
    return nullptr;
  }
}

extern void deleteONNXRunner(ONNXRunner *instance) {
  if (instance != nullptr) {
    delete instance;
  }
}

extern std::vector<float> runONNXModel(ONNXRunner *instance,
                                       std::vector<std::string> inputString,
                                       std::vector<int64_t> inputInt64,
                                       std::vector<float> inputFloat,
                                       int batchSize) {
  std::vector<float> nullResult;
  if (instance != nullptr) {
    return instance->runONNXModel(inputString, inputInt64, inputFloat,
                                  batchSize);
  } else {
    return nullResult;
  }
}

static bool startsWithPatt(const std::string &str, const std::string &pattern) {
  return str.rfind(pattern, 0) == 0;
}

static bool optionComparator(const std::string &str1, const std::string &str2) {
  for (size_t i = 0; i < str1.size() && i < str2.size(); ++i) {
    char c1 = str1[i];
    char c2 = str2[i];
    if (std::isupper(c1) && !std::isupper(c2)) {
      return 1;
    } else if (!std::isupper(c1) && std::isupper(c2)) {
      return 0;
    } else if (c1 != c2) {
      return c1 > c2;
    }
  }

  return str1.size() < str2.size();
}

static void truncatePrefix(const std::string &str, std::string &strPrefix) {
  const char prefixIndicator = '_';
  size_t idx = str.find(prefixIndicator);
  if (idx == std::string::npos)
    strPrefix = str;
  else
    strPrefix = str.substr(0, idx + 1);
}

static std::string encodeStringFeature(const std::string &str) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, str.c_str(), str.size());
  SHA256_Final(hash, &sha256);

  std::stringstream ss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
  }
  return ss.str();
}

static void preprocessData(std::vector<std::string> &inputString,
                           std::vector<int64_t> &inputInt64, int argcSW,
                           const char **argvSW, const char *mcpuOption,
                           int argcHW, int64_t *argvHW) {
  const char *outputOption = "-o";
  const char *macroPrefix = "-D";
  const char *needle = "--param";
  const char *flagPrefix = "-";
  const char *defaultOption = "-fdefault-option";
  const int defaultIntVal = 0;

  // Preprocessing string features.
  std::string outputFile = "";
  std::vector<std::string> inputStringRaw = {std::string(mcpuOption)};
  std::vector<std::string> macroOptions;
  for (int i = 0; i < argcSW; ++i) {
    std::string opt = std::string(argvSW[i]);
    if (startsWithPatt(opt, macroPrefix)) {
      macroOptions.push_back(opt);
    }
    if (i + 1 < argcSW && opt.compare(outputOption) == 0) {
      truncatePrefix(std::string(argvSW[i + 1]), outputFile);
    }
  }
  inputStringRaw.push_back(outputFile);

  std::sort(macroOptions.begin(), macroOptions.end(), optionComparator);
  for (size_t i = 0; i < macroOptions.size() &&
                     (int)inputStringRaw.size() < FEATURE_SIZE_STRING_OPT;
       ++i) {
    inputStringRaw.push_back(macroOptions[i]);
  }

  for (int i = 0;
       i < argcSW && (int)inputStringRaw.size() < FEATURE_SIZE_STRING_OPT;
       ++i) {
    std::string opt = std::string(argvSW[i]);
    if (!startsWithPatt(opt, macroPrefix) && !startsWithPatt(opt, needle) &&
        startsWithPatt(opt, flagPrefix)) {
      inputStringRaw.push_back(opt);
    }
  }

  for (int i = (int)inputStringRaw.size(); i < FEATURE_SIZE_STRING_OPT; ++i) {
    inputStringRaw.push_back(defaultOption);
  }
  for (size_t i = 0; i < inputStringRaw.size(); ++i) {
    inputString.push_back(encodeStringFeature(inputStringRaw[i]));
  }

  // Preprocessing int64 features.
  for (int i = 0; i < argcHW && i < FEATURE_SIZE_INT64_OPT; ++i) {
    inputInt64.push_back(argvHW[i]);
  }

  for (int i = (int)inputInt64.size(); i < FEATURE_SIZE_INT64_OPT; ++i) {
    inputInt64.push_back(defaultIntVal);
  }
}

extern int64_t runONNXModelOptimizer(int argcSW, const char **argvSW,
                                     const char *mcpuOption, int argcHW,
                                     int64_t *argvHW) {
  // Create model runner.
  ONNXRunner *instance = createONNXRunner(MODEL_PATH_OPT);
  if (instance == nullptr) {
    return -1;
  }

  // Preprocess data.
  std::vector<std::string> inputString;
  std::vector<int64_t> inputInt64;
  std::vector<float> inputFloat;
  preprocessData(inputString, inputInt64, argcSW, argvSW, mcpuOption, argcHW,
                 argvHW);

  // Run model.
  int64_t output;
  if (instance != nullptr) {
    output =
        instance->runONNXModelOptimizer(inputString, inputInt64, inputFloat, 1);
  }

  // Delete model runner.
  deleteONNXRunner(instance);
  return output;
}
} // namespace compilerONNXRunner
} // extern "C"
#endif
