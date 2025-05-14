from src.vectorizer.task_split_init import SplitGenTaskInit
from src.vectorizer.task_split_iterate import SplitGenTaskIterate
from src.vectorizer.task_split_feedback import SplitGenFeedback
from src.vectorizer.compilerTest import CompilerTest
from src.vectorizer.compilerTest import CorrectTest
from src.vectorizer.compilerTest import PerformanceTest
from src.vectorizer.compilerTest import reset_cache
from src.vectorizer.compilerTest import FormalVerification
from src.utils import retry_parse_fail_prone_cmd

CODEX = "code-davinci-002"
GPT3 = "text-davinci-003"
CHAT_GPT = "gpt-3.5-turbo"
GPT4 = "gpt-4"
DEEPSEEKCHAT = "deepseek-chat"
DEEPSEEKCODER = "deepseek-coder"

TencentDeepSeek = "deepseek-v3"

SiliconFlow = "deepseek-ai/DeepSeek-V3"
SiliconFlow2 = "Pro/deepseek-ai/DeepSeek-R1"

ProSiliconFlow = "Pro/deepseek-ai/DeepSeek-V3"
QwenSilicon = "Qwen/Qwen2.5-Coder-32B-Instruct"
QwenSilicon2 = "Qwen/Qwen2.5-72B-Instruct"
QwenSilicon3 = "deepseek-ai/DeepSeek-R1-Distill-Qwen-32B"

SYSU = "starlight/DeepSeek-R1-671B"
QWen = "Qwen2.5-Coder-32B-Instruct"
Nvidia = "meta/llama3-70b-instruct"

Ali = "llama3.1-8b-instruct"

ENGINE = os.getenv("ENGINE")
if ENGINE is None:
    ENGINE = ProSiliconFlow

@retry_parse_fail_prone_cmd
def iterative_vectorize(source_code: str, max_attempts: int, outputFileName: str) -> str:
    
    # initialize all the required components
    
    # generation of the first vectorized code
    task_init = SplitGenTaskInit(engine=ENGINE, prompt_examples="data/prompt/vectorize/init.jsonl")
    
    # getting feedback
    task_feedback = SplitGenFeedback(engine=ENGINE, prompt_examples="data/prompt/vectorize/feedback.jsonl")

    # iteratively improving the acronym
    task_iterate = SplitGenTaskIterate(engine=ENGINE, prompt_examples="data/prompt/vectorize/feedback.jsonl")
    
    
    # Initialize the task

    n_attempts = 0
    print("####################### SOURCE CODE #######################")
    print(f'''```c
{source_code}
```''')
    print("####################### SOURCE CODE #######################")
    clang_feedback = CompilerTest(vectorize_code=source_code)
    # print(clang_feedback)
    llm_feedback = ""

    unit_test_error = ""
    unit_test_output ="PASS"
    unit_test_feedback = ""

    while n_attempts < max_attempts:
        print(f"######################### ROUND {n_attempts} #########################\n")
        with open(outputFileName, "a") as of:
            of.write(f"\n# ROUND {n_attempts}\n")
        if n_attempts == 0:
            vectorize_code = task_init(code=source_code, compiler_feedback=clang_feedback, outputFileName=outputFileName)
        else:
            vectorize_code = task_iterate(code=source_code, vectorize_code=vectorize_code, llm_feedback = llm_feedback,compiler_feedback=clang_feedback,unit_test_feedback = unit_test_feedback, outputFileName=outputFileName)


        clang_feedback = CompilerTest(vectorize_code=vectorize_code)
  
        # 如果语义不正确，replay
        # 这里是不是应该再插入一个unit test
        print("######################## UNIT TEST ########################")
        unit_test_output, unit_test_error = CorrectTest("", source_code, vectorize_code,ENGINE)
        print("###################### UNIT TEST OUT ######################")
        print(unit_test_output)
        # print(unit_test_error)
        print("###################### UNIT TEST OUT ######################")
            
        unit_test_feedback = ("\nUnit Test analysis: \nSource code and optimized code semantics are inconsistent.\n" if unit_test_error != "" or "PASS" not in unit_test_output else "UNIT TEST PASS") # + "Performance compare:\n" + performance_test_output 
        

        llm_feedback = task_feedback(code=source_code, vectorize_code=vectorize_code, compiler_feedback=clang_feedback, outputFileName=outputFileName, unit_test_feedback = unit_test_feedback)
        # 生成的代码的依赖与源码的依赖是否相同

        if "PASS" in llm_feedback and "FAIL" not in llm_feedback:
            print("######################## UNIT TEST ########################")
            unit_test_output, unit_test_error = CorrectTest("", source_code, vectorize_code,ENGINE)
            print("###################### UNIT TEST OUT ######################")
            print(unit_test_output)
            # print(unit_test_error)
            print("###################### UNIT TEST OUT ######################")
            unit_test_feedback = ("\nUnit Test analysis: \nSource code and optimized code semantics are inconsistent.\n" if unit_test_error != "" or "PASS" not in unit_test_output else "UNIT TEST PASS") # + "Performance compare:\n" + performance_test_output 
            llm_feedback += unit_test_feedback
            if unit_test_error != "" or "PASS" not in unit_test_output:
                n_attempts += 1
                continue
            print("################### Formal Verification ###################")
            formalVerify = FormalVerification(source_code, vectorize_code)
            print(formalVerify)
            print("################### Formal Verification ###################")
            if "1 incorrect transformations" in formalVerify:
                n_attempts += 1
                continue
            #print("##################### PERFORMANCE TEST ####################")
            #performance_test_output, _ = PerformanceTest("", source_code, vectorize_code,ENGINE)
            #print("################### PERFORMANCE TEST OUT ###################")
            #print(performance_test_output)
            #print("################### PERFORMANCE TEST OUT ##################")
            with open(outputFileName, "a") as of:
                of.write(f"\n# FINAL CODE\n")
                of.write(f'''
```c
{vectorize_code}
```''')
            print("####################### FINAN OUTPUT ######################")
            print(vectorize_code)
            print("####################### FINAN OUTPUT ######################")
            return

        # print(f"{n_attempts} GEN> {vectorize_code}\n")

        # print(f"{n_attempts} FEEDBACK> {llm_feedback}")
        n_attempts += 1

if __name__ == "__main__":

    import sys
    import os
    # title = sys.argv[1]  # Light Amplification by Stimulated Emission of Radiation
    
    test_path = 'test_case'
    for filename in os.listdir(test_path):
        # if filename != 's256.c':
        #     continue
        file_path = os.path.join(test_path, filename)
        reset_cache()
        with open(file_path, 'r') as file:
            codeTest = file.read()
            # print(codeTest)
            log_dir = "log"
            output_file = "output.md"
            if not os.path.exists(log_dir):
                os.makedirs(log_dir)

            output_path = os.path.join(log_dir, output_file)
            if not os.path.exists(output_path):
                with open(output_path, 'w') as file:
                    file.write('')
            else:
                os.remove(output_path)
            #
            outputFileName = log_dir + "/" + output_file

            max_attempts = 20
            iterative_vectorize(
                source_code=codeTest,
                max_attempts=max_attempts,
                outputFileName=outputFileName,
            )
        print('#####################################################################################')
        print('#####################################################################################')
        print('#####################################################################################')
        print('#####################################################################################')
        print('#####################################################################################')
        
    # log_dir = "log"
    # output_file = "output.md"
    # if not os.path.exists(log_dir):
    #     os.makedirs(log_dir)

    # output_path = os.path.join(log_dir, output_file)
    # if not os.path.exists(output_path):
    #     with open(output_path, 'w') as file:
    #         file.write('')
    # else:
    #     os.remove(output_path)
    # #
    # outputFileName = log_dir + "/" + output_file

    # max_attempts = 100
    # iterative_vectorize(
    #     source_code=codeTest4,
    #     max_attempts=max_attempts,
    #     outputFileName=outputFileName,
    # )
    
     #print("\n ------ \n ".join(res))

