#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include "include/ONNXRunner.h"

namespace boltONNXRunner{

Ort::Value ONNXRunner::getInputValueFloat(Ort::Session *session, 
                                          std::vector<float> &input,
                                          int inputIdx) {
    auto typeInfo = session->GetInputTypeInfo(inputIdx);
    auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
    auto inputDims = tensorInfo.GetShape();
    std::replace_if(
        inputDims.begin(), inputDims.end(), [](int64_t &i) { return i < 0; }, 1);

    size_t inputTensorSize = std::accumulate(inputDims.begin(), inputDims.end(),
                                             1, std::multiplies<int>());
    auto memory_info =
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    auto inputTmp = Ort::Value::CreateTensor<float>(
        memory_info, input.data(), inputTensorSize, inputDims.data(),
        inputDims.size());
    auto inputTensor = &inputTmp;
    return inputTmp;
}

Ort::Value ONNXRunner::getInputValueString(Ort::AllocatorWithDefaultOptions allocator,
                                           Ort::Session *session, 
                                           std::vector<std::string> &input,
                                           int inputIdx) {
    auto typeInfo = session->GetInputTypeInfo(inputIdx);
    auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
    auto inputDims = tensorInfo.GetShape();

    std::replace_if(
        inputDims.begin(), inputDims.end(), [](int64_t &i) { return i < 0; }, 1);

    size_t inputTensorSize = std::accumulate(inputDims.begin(), inputDims.end(),
                                             1, std::multiplies<int>());
    const char* input_strings[inputTensorSize];
    for(int i = 0; i < inputTensorSize; i++) {
        input_strings[i] = input[i].c_str();
    }

    auto memory_info =
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    auto inputTmp = Ort::Value::CreateTensor(allocator, inputDims.data(), 
                                                 inputDims.size(), 
                                                 ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING);
    inputTmp.FillStringTensor(input_strings, inputTensorSize);
    auto inputTensor = &inputTmp;
    return inputTmp;
}

Ort::Value ONNXRunner::getInputValueInt64(Ort::Session *session, 
                                          std::vector<int64_t> &input,
                                          int inputIdx) {
    auto typeInfo = session->GetInputTypeInfo(inputIdx);
    auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
    auto inputDims = tensorInfo.GetShape();
    std::replace_if(
        inputDims.begin(), inputDims.end(), [](int64_t &i) { return i < 0; }, 1);

    size_t inputTensorSize = std::accumulate(inputDims.begin(), inputDims.end(),
                                             1, std::multiplies<int>());
    auto memory_info =
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    auto inputTmp = Ort::Value::CreateTensor<int64_t>(
        memory_info, input.data(), inputTensorSize, inputDims.data(),
        inputDims.size());
    auto inputTensor = &inputTmp;
    return inputTmp;
}

float ONNXRunner::runONNXModel(std::vector<std::string> input_string, std::vector<int64_t> input_int64, std::vector<float> input_float){
    Ort::AllocatorWithDefaultOptions allocator;

    //Try to get input;
    int input_count = session->GetInputCount();

    //Get input name
    std::vector<std::string> inputNameList;
    for (int i = 0; i < input_count; i++) {
        auto inputName = session->GetInputNameAllocated(i, allocator);
        auto inputNameStr = inputName.get();
        inputNameList.push_back(inputNameStr);
    }
    
    //Form input tensor(s)
    std::vector<Ort::Value> input_final;
    std::vector<const char *> inputNameStr_final;

    int currentIdx = 0;
    if(!input_string.empty()) {
        input_final.push_back(getInputValueString(allocator, session, input_string, currentIdx));
        currentIdx ++;
    }

    if(!input_int64.empty()) {
        input_final.push_back(getInputValueInt64(session, input_int64, currentIdx));
        currentIdx ++;
    }

    if(!input_float.empty()) {
        input_final.push_back(getInputValueFloat(session, input_float, currentIdx));
        currentIdx ++;
    }

    for (int i = 0; i < input_count; i++) {
        inputNameStr_final.push_back(inputNameList[i].c_str()); 
    }

    //Run the model
    int  output_count = session->GetOutputCount();
    std::vector<std::string> outputNameList;
    for (int i = 0; i < output_count; i++) {
        auto outputName = session->GetOutputNameAllocated(i, allocator);
        std::string outputNameStr = outputName.get();
        if(!outputNameStr.empty()) {
            outputNameList.push_back(outputNameStr); 
        } else {
            std::string outputNameDefault = "Output_" + std::to_string(i);
            outputNameList.push_back(outputNameDefault); 
        }
    }

    std::vector<const char *> outputNameStr_final;
    for(int i = 0; i < output_count; i++) {
        outputNameStr_final.push_back(outputNameList[i].c_str());
    }
    
    auto outputTensors =
      session->Run(Ort::RunOptions{nullptr}, inputNameStr_final.data(),
                   input_final.data(), input_count, outputNameStr_final.data(), output_count);

    //Try to get the result & return
    float* output_probability = outputTensors[0].GetTensorMutableData<float>();
    Ort::Value map_out = outputTensors[1].GetValue(static_cast<int>(0), allocator);

    Ort::Value keys_ort = map_out.GetValue(0, allocator);
    int64_t* keys_ret = keys_ort.GetTensorMutableData<int64_t>();
    Ort::Value values_ort = map_out.GetValue(1, allocator);
    float* values_ret = values_ort.GetTensorMutableData<float>();

    return *(values_ret + 1);
}

} // namespace boltONNXRunner

