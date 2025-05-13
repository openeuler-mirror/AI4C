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
    import os
    codeTestHard = '''
void set_points(float* dst, int* src, const int* divs, int divCount, int srcFixed,
                       int srcScalable, int srcStart, int srcEnd, float dstStart, float dstEnd,
                       bool isScalable) {
    float dstLen = dstEnd - dstStart;
    float scale;
    if (srcFixed <= dstLen) {
        // This is the "normal" case, where we scale the "scalable" patches and leave
        // the other patches fixed.
        scale = (dstLen - ((float) srcFixed)) / ((float) srcScalable);
    } else {
        // In this case, we eliminate the "scalable" patches and scale the "fixed" patches.
        scale = dstLen / ((float) srcFixed);
    }

    src[0] = srcStart;
    dst[0] = dstStart;
    for (int i = 0; i < divCount; i++) {
        src[i + 1] = divs[i];
        int srcDelta = src[i + 1] - src[i];
        float dstDelta;
        if (srcFixed <= dstLen) {
            dstDelta = isScalable ? scale * srcDelta : srcDelta;
        } else {
            dstDelta = isScalable ? 0.0f : scale * srcDelta;
        }
        dst[i + 1] = dst[i] + dstDelta;

        // Alternate between "scalable" and "fixed" patches.
        isScalable = !isScalable;
    }

    src[divCount + 1] = srcEnd;
    dst[divCount + 1] = dstEnd;
}
'''
    
    codeTest2 = '''
void s212 (int n , int *a , int *b , int *c ,int * d ) {
    for (int i = 0; i < n -1; i ++) {
        a [ i ] *= c [ i ];
        b [ i ] += a [ i + 1] * d [ i ];
    }
}    
'''



# 效果明显  loop not vectorized: unsafe dependent memory operations in loop.
    codeTest3 = '''
void s112(float *a, float *b, int iterations, int LEN_1D) {
    for (int nl = 0; nl < 3 * iterations; nl++) {
        for (int i = LEN_1D - 2; i >= 0; i--) {
            a[i + 1] = a[i] + b[i];
        }
    }
}
'''
    
    

# 无法通过，需要将内部循环拆成两部分  loop not vectorized: unsafe dependent memory operations in loop.
    codeTest4 = '''
void s1113(int iterations, int LEN_1D, float* a, float* b)
{
    for (int nl = 0; nl < 2*iterations; nl++) {
        for (int i = 0; i < LEN_1D; i++) {
            a[i] = a[LEN_1D/2] + b[i];
        }
    }
}    
'''
    codeTest = codeTest2

# loop not vectorized: unsafe dependent memory operations in loop.


# void s114_opt(int iterations, float aa[256][256], float bb[256][256]) {
#     for (int nl = 0; nl < 200 * (iterations / 256); nl++) {
#         // First loop: Compute the values that depend on aa[j][i] and store in a temporary array
#         float temp[256][256];
#         for (int i = 0; i < 256; i++) {
#             for (int j = 0; j < i; j++) {
#                 temp[i][j] = aa[j][i] + bb[i][j];
#             }
#         }

#         // Second loop: Update aa[i][j] with the computed values from the temporary array
#         for (int i = 0; i < 256; i++) {
#             for (int j = 0; j < i; j++) {
#                 aa[i][j] = temp[i][j];
#             }
#         }
#     }
# }

# 最终生成的结果因为cost model 没法向量化

    codeTest5 = '''
void s114(int iterations, float aa[256][256], float bb[256][256])
{
    for (int nl = 0; nl < 200*(iterations/(256)); nl++) {
        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < i; j++) {
                aa[i][j] = aa[j][i] + bb[i][j];
            }
        }
    }
}
'''
    
    
# loop not vectorized: unsafe dependent memory operations in loop.

# 未通过正确性验证，因为float运算后不能直接用==比较，所以验证一直无法通过
# void s115_opt(int iterations, float* a, float aa[256][256])
# {
#     for (int nl = 0; nl < 1000 * (iterations / 256); nl++) {
#         for (int j = 0; j < 256; j++) {
#             float temp = a[j];  // Cache a[j] to avoid repeated memory access
#             for (int i = j + 1; i < 256; i++) {
#                 a[i] -= aa[j][i] * temp;
#             }
#         }
#     }
# }
# 修改compilerTest后通过


    codeTest6 = '''
void s115(int iterations, float* a, float aa[256][256])
{
    for (int nl = 0; nl < 1000*(iterations/256); nl++) {
        for (int j = 0; j < 256; j++) {
            for (int i = j+1; i < 256; i++) {
                a[i] -= aa[j][i] * a[j];
            }
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop
# loop not vectorized: unsafe dependent memory operations in loop.


# void s116_opt(int iterations, int LEN_1D, float* a) {
#     for (int nl = 0; nl < iterations * 10; nl++) {
#         // Process the array in chunks of 5 elements
#         for (int i = 0; i < LEN_1D - 5; i += 5) {
#             float temp0 = a[i + 1] * a[i];
#             float temp1 = a[i + 2] * a[i + 1];
#             float temp2 = a[i + 3] * a[i + 2];
#             float temp3 = a[i + 4] * a[i + 3];
#             float temp4 = a[i + 5] * a[i + 4];

#             // Update the array elements after all computations are done
#             a[i] = temp0;
#             a[i + 1] = temp1;
#             a[i + 2] = temp2;
#             a[i + 3] = temp3;
#             a[i + 4] = temp4;
#         }
#     }
# }



    codeTest7 = '''
void s116(int iterations, int LEN_1D, float* a)
{
    for (int nl = 0; nl < iterations*10; nl++) {
        for (int i = 0; i < LEN_1D - 5; i += 5) {
            a[i] = a[i + 1] * a[i];
            a[i + 1] = a[i + 2] * a[i + 1];
            a[i + 2] = a[i + 3] * a[i + 2];
            a[i + 3] = a[i + 4] * a[i + 3];
            a[i + 4] = a[i + 5] * a[i + 4];
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop
# loop not vectorized: cannot identify array bounds
# 无法处理
    codeTest8 = '''
void s123(int iterations, int LEN_1D, float* a, float* b, float* c, float* d, float* e)
{
    int j;
    for (int nl = 0; nl < iterations; nl++) {
        j = -1;
        for (int i = 0; i < (LEN_1D/2); i++) {
            j++;
            a[j] = b[i] + d[i] * e[i];
            if (c[i] > (float)0.) {
                j++;
                a[j] = c[i] + d[i] * e[i];
            }
        }
    }
}
'''
    

# loop not vectorized: value that could not be identified as reduction is used outside the loop

# void s126_opt(int iterations, float bb[256][256], float cc[256][256], float * flat_2d_array)
# {
#     int k;
#     for (int nl = 0; nl < 10*(iterations/256); nl++) {
#         k = 1;
#         for (int i = 0; i < 256; i++) {
#             // Initialize the first element of bb[j][i] for each i
#             float temp = bb[0][i]; // Store the initial value to avoid repeated memory access

#             // Main loop with dependency preserved
#             for (int j = 1; j < 256; j++) {
#                 temp = temp + flat_2d_array[k - 1] * cc[j][i];
#                 bb[j][i] = temp;
#                 k++;
#             }
#             // Increment k for the next iteration of i
#             k++;
#         }
#     }
# }

    codeTest9 = '''
void s126(int iterations, float bb[256][256], float cc[256][256], float * flat_2d_array)
{
    int k;
    for (int nl = 0; nl < 10*(iterations/256); nl++) {
        k = 1;
        for (int i = 0; i < 256; i++) {
            for (int j = 1; j < 256; j++) {
                bb[j][i] = bb[j-1][i] + flat_2d_array[k-1] * cc[j][i];
                ++k;
            }
            ++k;
        }
    }
}
'''

# loop not vectorized: value that could not be identified as reduction is used outside the loop
# loop not vectorized: unsafe dependent memory operations in loop.


# void s141_opt(int iterations, float * flat_2d_array, float bb[256][256]) {
#     for (int nl = 0; nl < 200 * (iterations / 256); nl++) {
#         for (int i = 0; i < 256; i++) {
#             // Precompute the initial value of k
#             int k = (i + 1) * i / 2 + i;

#             // Inner loop remains unchanged to preserve the original semantics
#             for (int j = i; j < 256; j++) {
#                 flat_2d_array[k] += bb[j][i];
#                 k += j + 1;
#             }
#         }
#     }
# }


    codeTest10 = '''
void s141(int iterations, float * flat_2d_array, float bb[256][256])
{
    int k;
    for (int nl = 0; nl < 200*(iterations/256); nl++) {
        for (int i = 0; i < 256; i++) {
            k = (i+1) * ((i+1) - 1) / 2 + (i+1)-1;
            for (int j = i; j < 256; j++) {
                flat_2d_array[k] += bb[j][i];
                k += j+1;
            }
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop
# loop not vectorized: could not determine number of loop iterations
    codeTest11 = '''
void s161(int iterations, int LEN_1D, float* a, float* b,float*c, float*d,float*e)
{
    for (int nl = 0; nl < iterations/2; nl++) {
        for (int i = 0; i < LEN_1D-1; ++i) {
            if (b[i] < (float)0.) {
                goto L20;
            }
            a[i] = c[i] + d[i] * e[i];
            goto L10;
L20:
            c[i+1] = a[i] + d[i] * d[i];
L10:
            ;
        }
    }
}    
'''
    

# loop not vectorized: cannot identify array bounds

    codeTest12 = '''
void s1161(int iterations, int LEN_1D, float* a,float* b,float* c,float* d,float* e)
{
    for (int nl = 0; nl < iterations; nl++) {
        for (int i = 0; i < LEN_1D-1; ++i) {
            if (c[i] < (float)0.) {
                goto L20;
            }
            a[i] = c[i] + d[i] * e[i];
            goto L10;
L20:
            b[i] = a[i] + d[i] * d[i];
L10:
            ;
        }
    }
}

'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop
    codeTest13 = '''
void s211(int iterations, int LEN_1D, float*a, float*b,float*c,float*d,float*e)
{
    for (int nl = 0; nl < iterations; nl++) {
        for (int i = 1; i < LEN_1D-1; i++) {
            a[i] = b[i - 1] + c[i] * d[i];
            b[i] = b[i + 1] - e[i] * d[i];
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop
# loop not vectorized: unsafe dependent memory operations in loop.
    codeTest14 = '''
void s212 (int n , float *a , float *b , float *c ,float * d ) {
    for (int i = 0; i < n -1; i ++) {
        a [ i ] *= c [ i ];
        b [ i ] += a [ i + 1] * d [ i ];
    }
}    
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop
    codeTest15 = '''
void s1213(int iterations, int LEN_1D, float* a,float* b,float* c,float* d)
{
    for (int nl = 0; nl < iterations; nl++) {
        for (int i = 1; i < LEN_1D-1; i++) {
            a[i] = b[i-1]+c[i];
            b[i] = a[i+1]*d[i];
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop
    codeTest16 = '''
void s221(int iterations, int LEN_1D, float*a,float*b,float*c,float*d)
{
    for (int nl = 0; nl < iterations/2; nl++) {
        for (int i = 1; i < LEN_1D; i++) {
            a[i] += c[i] * d[i];
            b[i] = b[i - 1] + a[i] + d[i];
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop
    
    codeTest17 = '''
void s222(int iterations, int LEN_1D, float*a,float*b,float*c, float* e)
{
    for (int nl = 0; nl < iterations/2; nl++) {
        for (int i = 1; i < LEN_1D; i++) {
            a[i] += b[i] * c[i];
            e[i] = e[i - 1] * e[i - 1];
            a[i] -= b[i] * c[i];
        }
    }
}
'''
# loop not vectorized: value that could not be identified as reduction is used outside the loop
    
    codeTest18 = '''
void s231(int iterations, float aa[256][256], float bb[256][256])
{
    for (int nl = 0; nl < 100*(iterations/256); nl++) {
        for (int i = 0; i < 256; ++i) {
            for (int j = 1; j < 256; j++) {
                aa[j][i] = aa[j - 1][i] + bb[j][i];
            }
        }
    }
}
'''
    

# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest19 = '''
void s232(int iterations, float aa[256][256], float bb[256][256])
{
    for (int nl = 0; nl < 100*(iterations/(256)); nl++) {
        for (int j = 1; j < 256; j++) {
            for (int i = 1; i <= j; i++) {
                aa[j][i] = aa[j][i-1]*aa[j][i-1]+bb[j][i];
            }
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest20 = '''
void s233(int iterations, float aa[256][256], float bb[256][256], float cc[256][256])
{
    for (int nl = 0; nl < 100*(iterations/256); nl++) {
        for (int i = 1; i < 256; i++) {
            for (int j = 1; j < 256; j++) {
                aa[j][i] = aa[j-1][i] + cc[j][i];
            }
            for (int j = 1; j < 256; j++) {
                bb[j][i] = bb[j][i-1] + cc[j][i];
            }
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest21 = '''
void s2233(int iterations, float aa[256][256], float bb[256][256], float cc[256][256])
{
    for (int nl = 0; nl < 100*(iterations/256); nl++) {
        for (int i = 1; i < 256; i++) {
            for (int j = 1; j < 256; j++) {
                aa[j][i] = aa[j-1][i] + cc[j][i];
            }
            for (int j = 1; j < 256; j++) {
                bb[i][j] = bb[i-1][j] + cc[i][j];
            }
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest22 = '''
void s235(int iterations, float* a,float* b,float* c,float aa[256][256],float bb[256][256])
{
    for (int nl = 0; nl < 200*(iterations/256); nl++) {
        for (int i = 0; i < 256; i++) {
            a[i] += b[i] * c[i];
            for (int j = 1; j < 256; j++) {
                aa[j][i] = aa[j-1][i] + bb[j][i] * a[i];
            }
        }
    }
}
'''
    
# loop not vectorized: unsafe dependent memory operations in loop.

    codeTest23 = '''
void s241(int iterations, int LEN_1D, float* a,float* b,float* c,float* d)
{
    for (int nl = 0; nl < 2*iterations; nl++) {
        for (int i = 0; i < LEN_1D-1; i++) {
            a[i] = b[i] * c[i  ] * d[i];
            b[i] = a[i] * a[i+1] * d[i];
        }
    }
}
'''
    
#  loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest24 = '''
void s242(int iterations, int LEN_1D, float s1, float s2, float* a,float* b,float* c,float* d)
{
    for (int nl = 0; nl < iterations/5; nl++) {
        for (int i = 1; i < LEN_1D; ++i) {
            a[i] = a[i - 1] + s1 + s2 + b[i] + c[i] + d[i];
        }
    }
}
'''
    
# loop not vectorized: unsafe dependent memory operations in loop.

    codeTest25 = '''
void s244(int iterations, int LEN_1D, float* a,float* b,float* c,float* d)
{
    for (int nl = 0; nl < iterations; nl++) {
        for (int i = 0; i < LEN_1D-1; ++i) {
            a[i] = b[i] + c[i] * d[i];
            b[i] = c[i] + b[i];
            a[i+1] = b[i] + a[i+1] * d[i];
        }
    }
}
'''
    
# loop not vectorized: unsafe dependent memory operations in loop.

    codeTest26 = '''
void s1244(int LEN_1D, float* a,float* b,float* c,float* d)
{
    for (int i = 0; i < LEN_1D-1; i++) {
        a[i] = b[i] + c[i] * c[i] + b[i]*b[i] + c[i];
        d[i] = a[i] + a[i+1];
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest27 = '''
void s2251(int iterations, int LEN_1D, float* a,float* b,float* c,float* d, float* e)
{
    for (int nl = 0; nl < iterations; nl++) {
        float s = (float)0.0;
        for (int i = 0; i < LEN_1D; i++) {
            a[i] = s*e[i];
            s = b[i]+c[i];
            b[i] = a[i]+d[i];
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest28 = '''
void s256(int iterations, float* a,float* d, float aa[256][256],float bb[256][256])
{
    for (int nl = 0; nl < 10*(iterations/256); nl++) {
        for (int i = 0; i < 256; i++) {
            for (int j = 1; j < 256; j++) {
                a[j] = (float)1.0 - a[j - 1];
                aa[j][i] = a[j] + bb[j][i]*d[j];
            }
        }
    }
}
'''
    

# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest29 = '''
void s258(int iterations, float* a,float* b,float* c, float* d,float* e, float aa[256][256])
{
    float s;
    for (int nl = 0; nl < iterations; nl++) {
        s = 0.;
        for (int i = 0; i < 256; ++i) {
            if (a[i] > 0.) {
                s = d[i] * d[i];
            }
            b[i] = s * c[i] + d[i];
            e[i] = (s + (float)1.) * aa[0][i];
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop
    codeTest30 = '''
void s261(int iterations,int LEN_1D, float* a,float* b,float* c, float* d)
{
    float t;
    for (int nl = 0; nl < iterations; nl++) {
        for (int i = 1; i < LEN_1D; ++i) {
            t = a[i] + b[i];
            a[i] = t + c[i-1];
            t = c[i] * d[i];
            c[i] = t;
        }
    }
}
'''

# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest31 = '''
void s275(int iterations, float aa[256][256], float bb[256][256], float cc[256][256])
{
    for (int nl = 0; nl < 10*(iterations/256); nl++) {
        for (int i = 0; i < 256; i++) {
            if (aa[0][i] > (float)0.) {
                for (int j = 1; j < 256; j++) {
                    aa[j][i] = aa[j-1][i] + bb[j][i] * cc[j][i];
                }
            }
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop
# loop not vectorized: could not determine number of loop iterations

    codeTest32 = '''
int s277(int iterations,int LEN_1D, float* a,float* b,float* c, float* d, float* e)
{
    for (int nl = 0; nl < iterations; nl++) {
        for (int i = 0; i < LEN_1D-1; i++) {
                if (a[i] >= (float)0.) {
                    goto L30;
                }
                if (b[i] >= (float)0.) {
                    goto L30;
                }
                a[i] += c[i] * d[i];
L30:
                b[i+1] = c[i] + d[i] * e[i];
L20:
;
        }
    }
    return 20;
}
'''

# loop not vectorized: unsafe dependent memory operations in loop.

    codeTest33 = '''
void s281(int iterations,int LEN_1D, float* a,float* b,float* c)
{
    float x;
    for (int nl = 0; nl < iterations; nl++) {
        for (int i = 0; i < LEN_1D; i++) {
            x = a[LEN_1D-i-1] + b[i] * c[i];
            a[i] = x-(float)1.0;
            b[i] = x;
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest34 = '''
void s291(int iterations,int LEN_1D, float* a,float* b)
{
    int im1;
    for (int nl = 0; nl < 2*iterations; nl++) {
        im1 = LEN_1D-1;
        for (int i = 0; i < LEN_1D; i++) {
            a[i] = (b[i] + b[im1]) * (float).5;
            im1 = i;
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest35 = '''
void s292(int iterations,int LEN_1D, float* a,float* b)
{
    int im1, im2;
    for (int nl = 0; nl < iterations; nl++) {
        im1 = LEN_1D-1;
        im2 = LEN_1D-2;
        for (int i = 0; i < LEN_1D; i++) {
            a[i] = (b[i] + b[im1] + b[im2]) * (float).333;
            im2 = im1;
            im1 = i;
        }
    }
}
'''
    
# loop not vectorized: unsafe dependent memory operations in loop.

    codeTest36 = '''
void s293(int iterations,int LEN_1D, float* a)
{
    for (int nl = 0; nl < 4*iterations; nl++) {
        for (int i = 0; i < LEN_1D; i++) {
            a[i] = a[0];
        }
    }
}
'''
    

# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest37 = '''
void s2111(int iterations,float aa[256][256])
{
    for (int nl = 0; nl < 100*(iterations/(256)); nl++) {
        for (int j = 1; j < 256; j++) {
            for (int i = 1; i < 256; i++) {
                aa[j][i] = (aa[j][i-1] + aa[j-1][i])/1.9;
            }
        }
    }
}
'''
    

# loop not vectorized: call instruction cannot be vectorized
# loop not vectorized: instruction cannot be vectorized

    codeTest38 = '''
float test(float* A){
  float s = (float)0.0;
  for (int i = 0; i < 4; i++)
    s += A[i];
  return s;
}

float s31111(int iterations, float* a)
{
    float sum;
    for (int nl = 0; nl < 2000*iterations; nl++) {
        sum = (float)0.;
        sum += test(a);
        sum += test(&a[4]);
        sum += test(&a[8]);
        sum += test(&a[12]);
        sum += test(&a[16]);
        sum += test(&a[20]);
        sum += test(&a[24]);
        sum += test(&a[28]);
    }
    return sum;
}
'''

# loop not vectorized: cannot prove it is safe to reorder floating-point operations; allow reordering by specifying '#pragma clang loop vectorize(enable)' before the loop or by providing the compiler option '-ffast-math'.

    codeTest39 = '''
float s312(int iterations,int LEN_1D, float* a)
{

    float prod;
    for (int nl = 0; nl < 10*iterations; nl++) {
        prod = (float)1.;
        for (int i = 0; i < LEN_1D; i++) {
            prod *= a[i];
        }
    }
    return prod;
}
'''

# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest40 = '''
float s314(int iterations,int LEN_1D, float* a)
{
    float x;
    for (int nl = 0; nl < iterations*5; nl++) {
        x = a[0];
        for (int i = 0; i < LEN_1D; i++) {
            if (a[i] > x) {
                x = a[i];
            }
        }
    }
    return x;
}
'''

    
# loop not vectorized: value that could not be identified as reduction is used outside the loop
  
    codeTest41 = '''
float s315(int iterations,int LEN_1D, float* a)
{
    for (int i = 0; i < LEN_1D; i++)
        a[i] = (i * 7) % LEN_1D;

    float x, chksum;
    int index;
    for (int nl = 0; nl < iterations; nl++) {
        x = a[0];
        index = 0;
        for (int i = 0; i < LEN_1D; ++i) {
            if (a[i] > x) {
                x = a[i];
                index = i;
            }
        }
        chksum = x + (float) index;
    }
    return index + x + 1;
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest42 = '''
float s316(int iterations,int LEN_1D, float* a)
{
    float x;
    for (int nl = 0; nl < iterations*5; nl++) {
        x = a[0];
        for (int i = 1; i < LEN_1D; ++i) {
            if (a[i] < x) {
                x = a[i];
            }
        }
    }
    return x;
}
'''
    
# loop not vectorized: cannot prove it is safe to reorder floating-point operations;

    codeTest43 = '''
float s317(int iterations,int LEN_1D)
{
    float q;
    for (int nl = 0; nl < 5*iterations; nl++) {
        q = (float)1.;
        for (int i = 0; i < LEN_1D/2; i++) {
            q *= (float).99;
        }LLL
    }
    return q;
}
'''

# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest44 = '''
float s318(int iterations,int LEN_1D, float* a, int inc)
{
    int k, index;
    float max, chksum;
    for (int nl = 0; nl < iterations/2; nl++) {
        k = 0;
        index = 0;
        max = fabsf(a[0]);
        k += inc;
        for (int i = 1; i < LEN_1D; i++) {
            if (fabsf(a[k]) <= max) {
                goto L5;
            }
            index = i;
            max = fabsf(a[k]);
L5:
            k += inc;
        }
        chksum = max + (float) index;
    }
    return max + index + 1;
}

'''
    
# loop not vectorized: cannot prove it is safe to reorder floating-point operations;

    codeTest45 = '''
float s319(int iterations,int LEN_1D, float * a, float*b,float*c,float*d, float*e)
{
    float sum;
    for (int nl = 0; nl < 2*iterations; nl++) {
        sum = 0.;
        for (int i = 0; i < LEN_1D; i++) {
            a[i] = c[i] + d[i];
            sum += a[i];
            b[i] = c[i] + e[i];
            sum += b[i];
        }
    }
    return sum;
}

'''

# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest46 = '''
float s3110(int iterations,float aa[256][256])
{
    int xindex, yindex;
    float max, chksum;
    for (int nl = 0; nl < 100*(iterations/(256)); nl++) {
        max = aa[(0)][0];
        xindex = 0;
        yindex = 0;
        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < 256; j++) {
                if (aa[i][j] > max) {
                    max = aa[i][j];
                    xindex = i;
                    yindex = j;
                }
            }
        }
        chksum = max + (float) xindex + (float) yindex;
    }
    return max + xindex+1 + yindex+1;
}

'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest47 = '''
float s13110(int iterations,float aa[256][256])
{
    int xindex, yindex;
    float max, chksum;
    for (int nl = 0; nl < 100*(iterations/(256)); nl++) {
        max = aa[(0)][0];
        xindex = 0;
        yindex = 0;
        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < 256; j++) {
                if (aa[i][j] > max) {
                    max = aa[i][j];
                    xindex = i;
                    yindex = j;
                }
            }
        }
        chksum = max + (float) xindex + (float) yindex;
    }
    return max + xindex+1 + yindex+1;
}
'''

# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest48 = '''
float s3112(int iterations,int LEN_1D, float* a, float* b)
{
    float sum;
    for (int nl = 0; nl < iterations; nl++) {
        sum = (float)0.0;
        for (int i = 0; i < LEN_1D; i++) {
            sum += a[i];
            b[i] = sum;
        }
    }
    return sum;
}
'''

# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest49 = '''
float s3113(int iterations,int LEN_1D, float* a)
{
    float max;
    for (int nl = 0; nl < iterations*4; nl++) {
        max = fabsf(a[0]);
        for (int i = 0; i < LEN_1D; i++) {
            if ((fabsf(a[i])) > max) {
                max = fabsf(a[i]);
            }
        }
    }
    return max;
}
'''

# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest50 = '''
void s321(int iterations,int LEN_1D, float* a, float* b)
{
    for (int nl = 0; nl < iterations; nl++) {
        for (int i = 1; i < LEN_1D; i++) {
            a[i] += a[i-1] * b[i];
        }
    }
}
'''

# loop not vectorized: value that could not be identified as reduction is used outside the loop
 
    codeTest51 = '''
void s322(int iterations,int LEN_1D, float* a, float *b, float* c)
{
    for (int nl = 0; nl < iterations/2; nl++) {
        for (int i = 2; i < LEN_1D; i++) {
            a[i] = a[i] + a[i - 1] * b[i] + a[i - 2] * c[i];
        }
    }
}
'''

# loop not vectorized: value that could not be identified as reduction is used outside the loop
 
    codeTest52 = '''
void s323(int iterations,int LEN_1D, float* a, float *b, float* c, float* d, float* e)
{
    for (int nl = 0; nl < iterations/2; nl++) {
        for (int i = 1; i < LEN_1D; i++) {
            a[i] = b[i-1] + c[i] * d[i];
            b[i] = a[i] + c[i] * e[i];
        }
    }
}
'''
    
# loop not vectorized: could not determine number of loop iterations

    codeTest53 = '''
float s332(int iterations,int LEN_1D,int t, float* a)
{
    int index;
    float value;
    float chksum;
    for (int nl = 0; nl < iterations; nl++) {
        index = -2;
        value = -1.;
        for (int i = 0; i < LEN_1D; i++) {
            if (a[i] > t) {
                index = i;
                value = a[i];
                goto L20;
            }
        }
L20:
        chksum = value + (float) index;
    }
    return value;
}
'''

# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest54 = '''
void s341(int iterations,int LEN_1D, float* a, float *b)
{
    int j;
    for (int nl = 0; nl < iterations; nl++) {
        j = -1;
        for (int i = 0; i < LEN_1D; i++) {
            if (b[i] > (float)0.) {
                j++;
                a[j] = b[i];
            }
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop
  
    codeTest55 = '''
void s342(int iterations,int LEN_1D, float* a, float *b)
{
    int j = 0;
    for (int nl = 0; nl < iterations; nl++) {
        j = -1;
        for (int i = 0; i < LEN_1D; i++) {
            if (a[i] > (float)0.) {
                j++;
                a[i] = b[j];
            }
        }
    }
}
'''
    
# loop not vectorized: value that could not be identified as reduction is used outside the loop

    codeTest56 = '''
void s343(int iterations,float * flat_2d_array, float aa[256][256], float bb[256][256])
{
    int k;
    for (int nl = 0; nl < 10*(iterations/256); nl++) {
        k = -1;
        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < 256; j++) {
                if (bb[j][i] > (float)0.) {
                    k++;
                    flat_2d_array[k] = aa[j][i];
                }
            }
        }
    }
}
'''
    

# loop not vectorized: cannot prove it is safe to reorder floating-point operations;

    codeTest57 = '''
float s352(int iterations,int LEN_1D, float* a, float *b)
{
    float dot;
    for (int nl = 0; nl < 8*iterations; nl++) {
        dot = (float)0.;
        for (int i = 0; i < LEN_1D; i += 5) {
            dot = dot + a[i] * b[i] + a[i + 1] * b[i + 1] + a[i + 2]
                * b[i + 2] + a[i + 3] * b[i + 3] + a[i + 4] * b[i + 4];
        }
    }
    return dot;
}
'''

# loop not vectorized: loop contains a switch statement

    codeTest58 = '''
void s442(int iterations,int LEN_1D, float* a, float *b,float* c, float *d, float* e, int* indx)
{
    for (int nl = 0; nl < iterations/2; nl++) {
        for (int i = 0; i < LEN_1D; i++) {
            switch (indx[i]) {
                case 1:  goto L15;
                case 2:  goto L20;
                case 3:  goto L30;
                case 4:  goto L40;
            }
L15:
            a[i] += b[i] * b[i];
            goto L50;
L20:
            a[i] += c[i] * c[i];
            goto L50;
L30:
            a[i] += d[i] * d[i];
            goto L50;
L40:
            a[i] += e[i] * e[i];
L50:
            ;
        }
    }
}
'''
    
# loop not vectorized: library call cannot be vectorized. Try compiling with -fno-math-errno, -ffast-math, or similar flags
# loop not vectorized: instruction cannot be vectorized


    codeTest59 = '''
void s451(int iterations,int LEN_1D, float* a, float *b, float* c)
{
    for (int nl = 0; nl < iterations/5; nl++) {
        for (int i = 0; i < LEN_1D; i++) {
            a[i] = sinf(b[i]) + cosf(c[i]);
        }
    }
}
'''

# loop not vectorized: cannot prove it is safe to reorder floating-point operations;

    codeTest60 = '''
void s453(int iterations,int LEN_1D, float* a, float *b)
{
    float s;
    for (int nl = 0; nl < iterations*2; nl++) {
        s = 0.;
        for (int i = 0; i < LEN_1D; i++) {
            s += (float)2.;
            a[i] = s * b[i];
        }
    }
}
'''


# loop not vectorized: could not determine number of loop iterations

    codeTest61 = '''
void s481(int iterations,int LEN_1D, float* a, float *b, float *c, float* d)
{
    for (int nl = 0; nl < iterations; nl++) {
        for (int i = 0; i < LEN_1D; i++) {
            if (d[i] < (float)0.) {
                return;
            }
            a[i] += b[i] * c[i];
        }
    }
}
'''
    

# loop not vectorized: could not determine number of loop iterations

    codeTest62 = '''
void s482(int iterations,int LEN_1D, float* a, float *b, float *c)
{
    for (int nl = 0; nl < iterations; nl++) {
        for (int i = 0; i < LEN_1D; i++) {
            a[i] += b[i] * c[i];
            if (c[i] > b[i]) break;
        }
    }
}
'''
    
    codeTest63 = '''
bool AllPointsEq(const int pts[], int count) {
    for (int i = 1; i < count; ++i) {
        if (pts[0] != pts[i]) {
            return false;
        }
    }
    return true;
}
'''
    # import sys
    # import os
    # # title = sys.argv[1]  # Light Amplification by Stimulated Emission of Radiation
    
    # test_path = 'test_case'
    # for filename in os.listdir(test_path):
    #     # if filename != 's256.c':
    #     #     continue
    #     file_path = os.path.join(test_path, filename)
    #     reset_cache()
    #     with open(file_path, 'r') as file:
    #         codeTest = file.read()
    #         # print(codeTest)
    #         log_dir = "log"
    #         output_file = "output.md"
    #         if not os.path.exists(log_dir):
    #             os.makedirs(log_dir)

    #         output_path = os.path.join(log_dir, output_file)
    #         if not os.path.exists(output_path):
    #             with open(output_path, 'w') as file:
    #                 file.write('')
    #         else:
    #             os.remove(output_path)
    #         #
    #         outputFileName = log_dir + "/" + output_file

    #         max_attempts = 20
    #         iterative_vectorize(
    #             source_code=codeTest,
    #             max_attempts=max_attempts,
    #             outputFileName=outputFileName,
    #         )
    #     print('#####################################################################################')
    #     print('#####################################################################################')
    #     print('#####################################################################################')
    #     print('#####################################################################################')
    #     print('#####################################################################################')
        
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

    max_attempts = 100
    iterative_vectorize(
        source_code=codeTest4,
        max_attempts=max_attempts,
        outputFileName=outputFileName,
    )
    
     #print("\n ------ \n ".join(res))

