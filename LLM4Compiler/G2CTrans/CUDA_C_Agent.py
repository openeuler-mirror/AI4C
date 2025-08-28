import json
import Langchain_LLM as LL
import os
import Global_vars as GV
import subprocess
import glob
import re
import pandas as pd
import numpy as np

def readdata(filename):
    data_list = []
    with open(filename, 'r', encoding='utf-8') as f:
        data_list = [json.loads(line) for line in f]
    return data_list
#
def LaunchTrans():
    # features: ['id', 'cpp_code', 'cuda_code', 'consistent_cpp_inputs', 'consistent_cuda_inputs', 'cuda_wrapper', 'consistent_outputs'], num_rows: 233
    filename = "BabelTower/test/mono_cases.jsonl"
    C_CUDA_Data = readdata(filename)
    #
    agent = LL.TriTransformAgent()
    #import pdb
    #pdb.set_trace()
    num_rows = len(C_CUDA_Data)
    #
    for i in range(0, num_rows, 1):
        print(f"===================id: {i}====================")
        filename = "./CUDA_C_Results/" + str(i) + "_main.cpp"
        filename_cpp = "./CUDA_C_Results/" + str(i) + "_cpp_opt.cpp"
        if os.path.isfile(filename) and os.path.isfile(filename_cpp):
            print(f"id = {i} has been finished, jump to the next one!!")
            continue
        GV.ABS_SAVE_PATH = "./CUDA_C_Results/"
        GV.FILE_INDEX = i
        #
        test_code = C_CUDA_Data[i]["cuda_code"]
        ref_code = C_CUDA_Data[i]["cpp_code"]
        
        try:
            optimized_code = agent.run(test_code)
            print(f"Original Code: \n{test_code} \n   \nOptimized Code：\n{optimized_code}")
            
            if os.path.exists(filename) and os.path.isfile(filename):
                os.remove(filename)
            with open(filename, "a") as f:
                f.write(f"// original code(id = {i}): \n /*\n{test_code}\n*/\n")
                f.write(f"// optimized code: \n\n")
                f.write(optimized_code)
        except:
            print(f"LLM inference failed, jump to the next task: i = {i+1}")


if __name__ == "__main__":
    # launch GPU2CPU translation agent
    LaunchTrans()
