#ifndef COMPILER_PROFILE_ONNXRUNNER_H
#define COMPILER_PROFILE_ONNXRUNNER_H

#include "onnxruntime_c_api.h"
#include "onnxruntime_cxx_api.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <numeric>
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <vector>
#include <regex>

extern "C" {
namespace compilerONNXRunner {

const int FEATURE_SIZE_INT64_OPT = 6;
const int FEATURE_SIZE_STRING_OPT = 11;
char *ai_info = "7f454c460201010000000000000000000100b7000100000000000000000000000000000000000000a0020000000000000000000040000000000040000c000b00fd7bbea9fd030091000038d5e00f00f9e00f40f900fc58d3001c0012e01700b9e01740b91f20017140000054000000941f2003d5fd7bc2a8c0035fd6004743433a2028474e55292031302e332e3100001000000000000000017a520004781e011b0c1f002000000018000000000000003c00000000410e209d049e034ddedd0e0000000000000000000000000000000000000000000000000000000000000000010000000400f1ff000000000000000000000000000000000000000003000100000000000000000000000000000000000000000003000300000000000000000000000000000000000000000003000400000000000000000000000000000000000b00000000000100000000000000000000000000000000000000000003000600000000000000000000000000000000000e0000000000070014000000000000000000000000000000000000000300070000000000000000000000000000000000000000000300050000000000000000000000000000000000110000001200010000000000000000003c000000000000001d00000010000000000000000000000000000000000000000061695f696e666f2e63002478002464006765745f61695f696e666f0061626f72740000000000002c000000000000001b0100000b00000000000000000000001c0000000000000005010000020000000000000000000000002e73796d746162002e737472746162002e7368737472746162002e72656c612e74657874002e64617461002e627373002e636f6d6d656e74002e6e6f74652e474e552d737461636b002e72656c612e65685f6672616d6500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000010000000600000000000000000000000000000040000000000000003c000000000000000000000000000000040000000000000000000000000000001b0000000400000040000000000000000000000000000000100200000000000018000000000000000900000001000000080000000000000018000000000000002600000001000000030000000000000000000000000000007c0000000000000000000000000000000000000000000000010000000000000000000000000000002c00000008000000030000000000000000000000000000007c0000000000000000000000000000000000000000000000010000000000000000000000000000003100000001000000300000000000000000000000000000007c0000000000000013000000000000000000000000000000010000000000000001000000000000003a00000001000000000000000000000000000000000000008f0000000000000000000000000000000000000000000000010000000000000000000000000000004f0000000100000002000000000000000000000000000000900000000000000038000000000000000000000000000000080000000000000000000000000000004a000000040000004000000000000000000000000000000028020000000000001800000000000000090000000700000008000000000000001800000000000000010000000200000000000000000000000000000000000000c80000000000000020010000000000000a0000000a00000008000000000000001800000000000000090000000300000000000000000000000000000000000000e801000000000000230000000000000000000000000000000100000000000000000000000000000011000000030000000000000000000000000000000000000040020000000000005900000000000000000000000000000001000000000000000000000000000000";

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
  int64_t runONNXModelLTO(std::vector<std::string> inputString,
                          std::vector<int64_t> inputInt64,
                          std::vector<float> inputFloat, int batchSize);

private:
  static Ort::Value
  getInputValueFloat(Ort::Session *session,
                     std::vector<float> &input, int inputIdx,
                     int batchSize);

  static Ort::Value
  getInputValueString(Ort::AllocatorWithDefaultOptions allocator,
                      Ort::Session *session, std::vector<std::string> &input,
                      int inputIdx, int batchSize);

  static Ort::Value
  getInputValueInt64(Ort::Session *session,
                     std::vector<int64_t> &input,
                     int inputIdx, int batchSize);

  Ort::SessionOptions sessionOptions;
  Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "test"};
  Ort::Session *session;
};

extern ONNXRunner *createONNXRunner(const char *modelPath) {
  std::filesystem::path filePath(modelPath);
  if (std::filesystem::exists(filePath)) {
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

bool startsWith(const std::string &str, const std::string &prefix) {
    if (str.size() < prefix.size()) {
        return false;
    }
    return str.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string &str, const std::string &suffix) {
    if (str.size() < suffix.size()) {
        return false;
    }
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::vector<std::string> split(const std::string& str, const std::string& delimiters) {
    std::regex regex("[" + delimiters + "]");
    std::sregex_token_iterator it(str.begin(), str.end(), regex, -1);
    std::sregex_token_iterator end;
    std::vector<std::string> tokens;
    while (it != end) {
        tokens.push_back(*it++);
    }
    return tokens;
}

// Preprocess data for LTO model.
static void preprocessLTOData(std::vector<std::string> &inputString, std::string input) {
  std::vector<std::string> words;
  std::istringstream iss(input);
  std::string word;
  std::string document;

  while (iss >> word) {
    words.push_back(word);
  }

  int length = words.size();
  int minimumLinkFileLength = 3;
  for (int i = 0; i < length; i++) {
    if (endsWith(words[i], ".a") || endsWith(words[i], ".so") || endsWith(words[i], ".o")) {
      std::string filename = words[i].substr(words[i].find_last_of("/") + 1);
      std::string commands = filename.substr(0, filename.find_last_of("."));
      std::vector<std::string> command_parts = split(commands, "-_./|");
      for (const auto &ele : command_parts) {
        if (ele.size() > minimumLinkFileLength) {
          inputString.push_back(ele);
	}
      }
    }
  }
  std::sort(inputString.begin(), inputString.end());
  auto last = std::unique(inputString.begin(), inputString.end());
  inputString.erase(last, inputString.end());
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

static bool findOptimizerModelPath(const std::string &modelRelPath,
                                   std::string &optModelPath,
                                   const char *envName = "LD_LIBRARY_PATH") {

  const char *paths = std::getenv(envName);
  std::istringstream envPaths{paths ? paths : ""};
  std::vector<std::string> modelPathList;

  // Split environment variables and concatenate complete model paths.
  std::string modelPath;
  while (std::getline(envPaths, modelPath, ':')) {
    if (modelPath[modelPath.size() - 1] != '/') {
      modelPath += '/';
    }
    modelPath += modelRelPath;
    modelPathList.push_back(modelPath);
  }

  for (const auto &modelPath : modelPathList) {
    std::filesystem::path filePath(modelPath);
    if (std::filesystem::exists(filePath)) {
      optModelPath = modelPath;
      return true;
    }
  }
  return false;
}

extern int64_t runONNXModelOptimizer(int argcSW, const char **argvSW,
                                     const char *mcpuOption, int argcHW,
                                     int64_t *argvHW) {
  // Create model runner.
  std::string optModelPath;
  std::string modelRelPath = "AI4C/optimizer.onnx";
  const char *envName = "LD_LIBRARY_PATH";
  if (!findOptimizerModelPath(modelRelPath, optModelPath, envName)) {
    return -1;
  }

  ONNXRunner *instance = createONNXRunner(optModelPath.c_str());
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

// Interface for LTO model.
// Add one more param to unify interface.
extern int64_t runONNXModelLTO(char* link_files)
{
  // Create AI models.
  std::string optModelPath;
  // Should make sure the path for LTO model.
  std::string modelRelPath = "AI4C/auto_lto.onnx";
  const char *envName = "LD_LIBRARY_PATH";
  if (!findOptimizerModelPath(modelRelPath, optModelPath, envName)) {
    return -1;
  }

  ONNXRunner *instance = createONNXRunner(optModelPath.c_str());
  if (instance == nullptr) {
    return -1;
  }

  std::vector<int64_t> inputInt64;
  std::vector<float> inputFloat;
  std::vector<std::string> inputString;
  preprocessLTOData(inputString, link_files);
  int64_t output;
  if (instance != nullptr) {
    output =
        instance->runONNXModelLTO(inputString, inputInt64, inputFloat, 1);
  }
  // Delete model runner.
  deleteONNXRunner(instance);
  return output;
}

} // namespace compilerONNXRunner
} // extern "C"
#endif
