#include "include/ONNXRunner.h"
#include <algorithm>
#include <numeric>
#include <vector>

namespace compilerONNXRunner {

Ort::Value ONNXRunner::getInputValueFloat(Ort::Session *session,
                                          std::vector<float> &input,
                                          int inputIdx, int batchSize) {
  auto typeInfo = session->GetInputTypeInfo(inputIdx);
  auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
  auto inputDims = tensorInfo.GetShape();
  std::replace_if(
      inputDims.begin(), inputDims.end(), [](int64_t &i) { return i < 0; }, 1);

  size_t inputTensorSize = std::accumulate(inputDims.begin(), inputDims.end(),
                                           1, std::multiplies<int>());
  // try to add batch size
  inputDims[0] = batchSize;
  inputTensorSize = inputTensorSize * batchSize;
  auto memoryInfo =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
  auto inputTmp =
      Ort::Value::CreateTensor<float>(memoryInfo, input.data(), inputTensorSize,
                                      inputDims.data(), inputDims.size());
  auto inputTensor = &inputTmp;
  return inputTmp;
}

Ort::Value ONNXRunner::getInputValueString(
    Ort::AllocatorWithDefaultOptions allocator, Ort::Session *session,
    std::vector<std::string> &input, int inputIdx, int batchSize) {
  auto typeInfo = session->GetInputTypeInfo(inputIdx);
  auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
  auto inputDims = tensorInfo.GetShape();

  std::replace_if(
      inputDims.begin(), inputDims.end(), [](int64_t &i) { return i < 0; }, 1);

  size_t inputTensorSize = std::accumulate(inputDims.begin(), inputDims.end(),
                                           1, std::multiplies<int>());
  inputDims[0] = batchSize;
  inputTensorSize = inputTensorSize * batchSize;
  const char *inputStrings[inputTensorSize];
  for (int i = 0; i < inputTensorSize; i++) {
    inputStrings[i] = input[i].c_str();
  }

  auto memoryInfo =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
  auto inputTmp =
      Ort::Value::CreateTensor(allocator, inputDims.data(), inputDims.size(),
                               ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING);
  inputTmp.FillStringTensor(inputStrings, inputTensorSize);
  auto inputTensor = &inputTmp;
  return inputTmp;
}

Ort::Value ONNXRunner::getInputValueInt64(Ort::Session *session,
                                          std::vector<int64_t> &input,
                                          int inputIdx, int batchSize) {
  auto typeInfo = session->GetInputTypeInfo(inputIdx);
  auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
  auto inputDims = tensorInfo.GetShape();
  std::replace_if(
      inputDims.begin(), inputDims.end(), [](int64_t &i) { return i < 0; }, 1);

  size_t inputTensorSize = std::accumulate(inputDims.begin(), inputDims.end(),
                                           1, std::multiplies<int>());
  inputDims[0] = batchSize;
  inputTensorSize = inputTensorSize * batchSize;
  auto memoryInfo =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
  auto inputTmp = Ort::Value::CreateTensor<int64_t>(
      memoryInfo, input.data(), inputTensorSize, inputDims.data(),
      inputDims.size());
  auto inputTensor = &inputTmp;
  return inputTmp;
}

std::vector<float>
ONNXRunner::runONNXModel(std::vector<std::string> inputString,
                         std::vector<int64_t> inputInt64,
                         std::vector<float> inputFloat, int batchSize) {
  Ort::AllocatorWithDefaultOptions allocator;

  // Get input count
  int inputCount = session->GetInputCount();

  // Get input name
  std::vector<std::string> inputNameList;
  for (int i = 0; i < inputCount; i++) {
    auto inputName = session->GetInputNameAllocated(i, allocator);
    auto inputNameStr = inputName.get();
    inputNameList.push_back(inputNameStr);
  }

  // Form input tensor(s)
  std::vector<Ort::Value> inputFinal;
  std::vector<const char *> inputNameStrFinal;
  int currentIdx = 0;
  if (!inputString.empty()) {
    inputFinal.push_back(getInputValueString(allocator, session, inputString,
                                             currentIdx, batchSize));
    currentIdx++;
  }

  if (!inputInt64.empty()) {
    inputFinal.push_back(
        getInputValueInt64(session, inputInt64, currentIdx, batchSize));
    currentIdx++;
  }

  if (!inputFloat.empty()) {
    inputFinal.push_back(
        getInputValueFloat(session, inputFloat, currentIdx, batchSize));
    currentIdx++;
  }

  for (int i = 0; i < inputCount; i++) {
    inputNameStrFinal.push_back(inputNameList[i].c_str());
  }

  // Run model
  int outputCount = session->GetOutputCount();
  std::vector<std::string> outputNameList;
  for (int i = 0; i < outputCount; i++) {
    auto outputName = session->GetOutputNameAllocated(i, allocator);
    std::string outputNameStr = outputName.get();
    if (!outputNameStr.empty()) {
      outputNameList.push_back(outputNameStr);
    } else {
      std::string outputNameDefault = "Output_" + std::to_string(i);
      outputNameList.push_back(outputNameDefault);
    }
  }

  std::vector<const char *> outputNameStrFinal;
  for (int i = 0; i < outputCount; i++) {
    outputNameStrFinal.push_back(outputNameList[i].c_str());
  }

  auto outputTensors = session->Run(
      Ort::RunOptions{nullptr}, inputNameStrFinal.data(), inputFinal.data(),
      inputCount, outputNameStrFinal.data(), outputCount);

  // Get result and return
  std::vector<float> probs;
  float *outputProbability = outputTensors[0].GetTensorMutableData<float>();
  for (int i = 0; i < batchSize; i++) {
    Ort::Value mapOut =
        outputTensors[1].GetValue(static_cast<int>(i), allocator);
    Ort::Value keysOrt = mapOut.GetValue(0, allocator);
    int64_t *keysRet = keysOrt.GetTensorMutableData<int64_t>();
    Ort::Value valuesOrt = mapOut.GetValue(1, allocator);
    float *valuesRet = valuesOrt.GetTensorMutableData<float>();
    probs.push_back((*(valuesRet + 1)));
  }

  return probs;
}

int64_t ONNXRunner::runONNXModelOptimizer(std::vector<std::string> inputString,
                                          std::vector<int64_t> inputInt64,
                                          std::vector<float> inputFloat,
                                          int batchSize) {
  Ort::AllocatorWithDefaultOptions allocator;

  // Get input count
  int inputCount = session->GetInputCount();
  std::vector<std::vector<int64_t>> inputInt64Tensors(FEATURE_SIZE_INT64_OPT);
  std::vector<std::string> inputStringTensor;
  std::vector<Ort::Value> inputFinal;

  for (int i = 0; i < FEATURE_SIZE_INT64_OPT; i++) {
    inputInt64Tensors[i].push_back(inputInt64[i]);
    inputFinal.push_back(
        getInputValueInt64(session, inputInt64Tensors[i], i, batchSize));
  }

  for (int i = FEATURE_SIZE_INT64_OPT;
       i < FEATURE_SIZE_INT64_OPT + FEATURE_SIZE_STRING_OPT; i++) {
    inputStringTensor.clear();
    inputStringTensor.push_back(inputString[i - FEATURE_SIZE_INT64_OPT]);
    inputFinal.push_back(getInputValueString(allocator, session,
                                             inputStringTensor, i, batchSize));
  }

  // Get input name from model
  std::vector<std::string> inputNameList;
  for (int i = 0; i < inputCount; i++) {
    auto inputName = session->GetInputNameAllocated(i, allocator);
    auto inputNameStr = inputName.get();
    inputNameList.push_back(inputNameStr);
  }

  // Form input tensor(s)
  std::vector<const char *> inputNameStrFinal;
  for (int i = 0; i < inputCount; i++) {
    inputNameStrFinal.push_back(inputNameList[i].c_str());
  }

  // Run model
  int outputCount = session->GetOutputCount();
  std::vector<std::string> outputNameList;
  for (int i = 0; i < outputCount; i++) {
    auto outputName = session->GetOutputNameAllocated(i, allocator);
    std::string outputNameStr = outputName.get();
    if (!outputNameStr.empty()) {
      outputNameList.push_back(outputNameStr);
    } else {
      std::string outputNameDefault = "Output_" + std::to_string(i);
      outputNameList.push_back(outputNameDefault);
    }
  }

  std::vector<const char *> outputNameStrFinal;
  for (int i = 0; i < outputCount; i++) {
    outputNameStrFinal.push_back(outputNameList[i].c_str());
  }

  auto outputTensors = session->Run(
      Ort::RunOptions{nullptr}, inputNameStrFinal.data(), inputFinal.data(),
      inputCount, outputNameStrFinal.data(), outputCount);

  // Get result and return
  int64_t label = 0;
  for (int i = 0; i < batchSize; i++) {
    int64_t *outputLabel = outputTensors[0].GetTensorMutableData<int64_t>();
    label = *outputLabel;
  }

  return label;
}

int64_t ONNXRunner::runONNXModelLTO (std::vector<std::string> inputString,
                                     std::vector<int64_t> inputInt64,
                                     std::vector<float> inputFloat,
                                     int batchSize) {
  
  Ort::AllocatorWithDefaultOptions allocator;

  int size = inputString.size();
    
  // Get input count
  int inputCount = session->GetInputCount();
  std::vector<std::vector<int64_t>> inputInt64Tensors(FEATURE_SIZE_INT64_OPT);
  std::vector<std::string> inputStringTensor;

   // Get input name
  std::vector<std::string> inputNameList;
  for (int i = 0; i < inputCount; i++) {
    auto inputName = session->GetInputNameAllocated(i, allocator);
    auto inputNameStr = inputName.get();
    inputNameList.push_back(inputNameStr);
  }

  // Form input tensor(s)
  std::vector<Ort::Value> inputFinal;
  std::vector<const char *> inputNameStrFinal;
  int currentIdx = 0;
  if (!inputString.empty()) {
    inputFinal.push_back(getInputValueString(allocator, session, inputString,
                                             currentIdx, batchSize));
    currentIdx++;
  }

  if (!inputInt64.empty()) {
    inputFinal.push_back(
        getInputValueInt64(session, inputInt64, currentIdx, batchSize));
    currentIdx++;
  }

  if (!inputFloat.empty()) {
    inputFinal.push_back(
        getInputValueFloat(session, inputFloat, currentIdx, batchSize));
    currentIdx++;
  }

  for (int i = 0; i < inputCount; i++) {
    inputNameStrFinal.push_back(inputNameList[i].c_str());
  }

  // Run model
  int outputCount = session->GetOutputCount();
  std::vector<std::string> outputNameList;
  for (int i = 0; i < outputCount; i++) {
    auto outputName = session->GetOutputNameAllocated(i, allocator);
    std::string outputNameStr = outputName.get();
    if (!outputNameStr.empty()) {
      outputNameList.push_back(outputNameStr);
    } else {
      std::string outputNameDefault = "Output_" + std::to_string(i);
      outputNameList.push_back(outputNameDefault);
    }
  }

  std::vector<const char *> outputNameStrFinal;
  for (int i = 0; i < outputCount; i++) {
    outputNameStrFinal.push_back(outputNameList[i].c_str());
  }

  auto outputTensors = session->Run(
    Ort::RunOptions{nullptr}, inputNameStrFinal.data(), inputFinal.data(),
    inputCount, outputNameStrFinal.data(), outputCount);
  
  int64_t label = 0;
  for (int i = 0; i < batchSize; i++) {
    int64_t *outputLabel = outputTensors[0].GetTensorMutableData<int64_t>();
    label = *outputLabel;
  }

  return label;
}


} // namespace compilerONNXRunner
