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

def compilecpp(source_cpp):
    clang_path = os.getenv("CLANG_PATH", "clang++")
    compile_cmd = [
            clang_path,
            "-O0",
            "-S", "-emit-llvm",
            source_cpp
            ]
    subprocess.run(compile_cmd, capture_output = True, text = True, check = Tr


def extract_function_name(code):
    pattern = r'''
        (?:[\w\s\*&const]+?\s+)
        (?:(?:\w+::)*)
        ([a-za-z_][a-za-z0-9_]*)
        \s*\([^)]*\)\s*
        \{
    '''
    match = re.search(pattern, code, re.verbose)

    if match:
        return match.group(1)
    return none

def formalverify(src_name, tar_name):
    alive_tv_cmd = ["alive-tv", src_name, tar_name]
    res = subprocess.run(alive_tv_cmd, capture_output = True, text = True, check = True)
    return res.stdout

def delete_files_by_pattern(directory, pattern):
    full_pattern = os.path.join(directory, pattern)
    files_to_delete = glob.glob(full_pattern)

    if not files_to_delete:
        print(f"No matched files: {full_pattern}")
        return

    for file_path in files_to_delete:
        os.remove(file_path)
        print("has deleted: {file_path}")

def deletefiles(filepath, N):
    for i in range(N):
        delete_files_by_pattern(filepath, f"{i}_*.cpp")


def save_array_to_excel(array, filename):
    df = pd.DataFrame(array)
    df.to_excel(f"{filename}.xlsx", index=False, header=False)


def semantic_verification():
    filename = "BabelTower/test/mono_cases.jsonl"
    filepath = "./CUDA_C_Results/"
    C_CUDA_Data = readdata(filename)
    num_rows = len(C_CUDA_Data)
    fileverify = "formalverification.log"
    CorrectVeriCount = 0
    FailCount = 0
    filename_results = "formalVresults"
    verifysummary = np.zeros((num_rows, 2))
    verifysummary[:, 0] = np.arange(num_rows)
    for i in range(num_rows):
        print(f"============i: {i} ================")
        filename_opt = os.path.join(filepath, f"{i}_cpp_opt.cpp")
        with open(filename_opt, "r") as f:
            pred_code = f.read()
        #
        ref_code = C_CUDA_Data[i]["cpp_code"]
        pred_name = extract_function_name(pred_code)
        ref_name = extract_function_name(ref_code)
        if not pred_name or not ref_name:
            print("Error: Failed to extract function name!")
            continue
        #
        try:
            new_ref_code = ref_code.replace(ref_name, pred_name)
        except:
            print(f"==========i: {i} failed, jump to the next one")
            continue

        filename_ref = os.path.join(filepath, f"{i}_cpp.cpp")
        with open(filename_ref, "w") as f:
            f.write(new_ref_code)
        #
        try:
            compilecpp(filename_ref)
            compilecpp(filename_opt)
            src_name = str(i) + "_cpp.ll"
            tar_name = str(i) + "_cpp_opt.ll"
            verires = formalverify(src_name, tar_name)
            print(f"===========i: {i}===========")
            print(f"filename_ref: \n{ref_code}")
            print(f"filename_opt: \n{pred_code}")
            print(f"FormalVerification: \n{verires}")
            with open(fileverify, "a") as f:
                f.write(f"===========i: {i}=======\n{verires}\n\n")
            if "1 correct transformations" in verires:
                CorrectVeriCount += 1
                verifysummary[i, 1] = 1
            else:
                verifysummary[i, 1] = -1
        except:
            print(f"i: {i} Failed due to compilation or alive2 verification, jump to the next!")
            FailCount += 1
            continue
        print("=========================Transformation Summary=============")
        print(f"Correct Formal: {CorrectVeriCount}/{num_rows}")
        print(f"Failed Formal: {FailCount}/{num_rows}")
    save_array_to_excel(verifysummary, filename_results)

if __name__ == "__main__":
    # launch GPU2CPU translation agent
    LaunchTrans()
    # verify the semantic equivalence of the translated CPU code with the reference CPU code.
    semantic_verification()
    #filepath = "./CUDA_C_Results/"
    #deletefiles(filepath, 18)
