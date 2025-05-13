import subprocess
import os
import re
from prompt_lib.backends import openai_api

clang_path = os.getenv("CLANG_PATH", "clang")

# Test if it can compile correctly
def CompilerTest(vectorize_code : str) -> str:
    include_head = '''
#include<stdlib.h>
#include<stdio.h>
#include<math.h>
'''
    vectorize_code = include_head + vectorize_code
    # Write to a temporary file
    with open("temp_code.cpp", "w") as f:
        f.write(vectorize_code)
    
    # Compilation command, including compiler options
    compile_command = [
        clang_path,                  # Use BiShengCompiler
        "-O3",                    # Optimization level
        "-S", "-emit-llvm",
        "-Rpass-analysis=loop-vectorize",  # Add LLVM debug options
        "-Rpass=loop-vectorize",
        "-ffast-math",
        # "-Rpass-analysis=slp-vectorize",
        # "-Rpass=slp-vectorize",
        "temp_code.cpp",            # 目标文件
    ]
    # Call clang to compile and capture stderr
    try:
        result = subprocess.run(
            compile_command, 
            capture_output=True,      # Capture stdout and stderr
            text=True,               # Handle output as text
            check=True               # Raise an exception if compilation fails
        )
        # Return the standard error output of the compilation
        return result.stderr
    
    except subprocess.CalledProcessError as e:
        # If clang compilation fails, return the error message
        return e.stderr

def FormalVerification(source_code: str, target_code: str):
    include_head = '''
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
'''
    head_index_src = source_code.find(')')
    if head_index_src != -1:
        src_code_head = source_code[:head_index_src+1]

    head_index_tgt = target_code.find(')')
    if head_index_tgt != -1:
        tgt_code_body = target_code[head_index_tgt+1:]

    target_code = src_code_head + tgt_code_body
    
    # Prepare source and target code by adding necessary includes
    source_code = include_head + source_code
    target_code = include_head + target_code

    # Write source and target code to temporary .cpp files
    with open("source_code.c", "w") as f:
        f.write(source_code)
    with open("target_code.c", "w") as f:
        f.write(target_code)

    # Compile the source and target code into LLVM IR using clang
    compile_command_source = [
        clang_path,
        "-O0",
        "-S", "-emit-llvm",
        "source_code.c",
    ]
    compile_command_target = [
        clang_path,
        "-O0",
        "-S", "-emit-llvm",
        "target_code.c",
    ]

    try:
        # Compile source code into source.ll
        subprocess.run(compile_command_source, capture_output=True, text=True, check=True)
        # Compile target code into target.ll
        subprocess.run(compile_command_target, capture_output=True, text=True, check=True)

        # Execute alive-tv with source.ll and target.ll
        alive_tv_command = [
            "alive-tv", "source_code.ll", "target_code.ll"
        ]
        result = subprocess.run(
            alive_tv_command,
            capture_output=True,
            text=True,
            check=True
        )

        # Return the output from alive-tv execution
        return result.stdout

    except subprocess.CalledProcessError as e:
        # If compilation or alive-tv execution fails, return the error message
        return f"Error occurred: {e.stderr}"

    finally:
        # Clean up temporary files
        if os.path.exists("source_code.c"):
            os.remove("source_code.c")
        if os.path.exists("target_code.c"):
            os.remove("target_code.c")
        if os.path.exists("source_code.ll"):
            os.remove("source_code.ll")
        if os.path.exists("target_code.ll"):
            os.remove("target_code.ll")



# generate test code by LLM
def generate_test_code(prompt: str, engine: str) -> str:
    try:
        # call LLM
        response = openai_api.OpenaiAPIWrapper.call(
            prompt=prompt,
            engine=engine,
            max_tokens=4096,
            stop_token=None,
            temperature=1.0,
        )

        generated_code = openai_api.OpenaiAPIWrapper.get_first_response(response)
        
        # deal with deepseek-R1
        if "</think>" in generated_code:
            generated_code = generated_code.split("</think>", 1)[1]
        pattern = r'```c(.*?)```'
        generated_code = re.findall(pattern, generated_code, re.DOTALL)[0]
        return generated_code
    except Exception as e:
        return f"Error generating test code: {e}"

# execute test code
def execute_test_code(test_code: str):
    try:
        # write test code
        with open("test_code.c", "w") as f:
            f.write(test_code)
        
        # compile test code
        compile_command = [
            clang_path, "-o", "test_program", "test_code.c", "-O3", "-lm"
        ]
        subprocess.run(compile_command, check=True, capture_output=True)
        
        # execute test code
        result = subprocess.run("./test_program", capture_output=True, text=True)
        
        return result.stdout, result.stderr
    except subprocess.CalledProcessError as e:
        return f"Test execution failed: {e.stderr.decode('utf-8')}", ""
    finally:
        if os.path.exists("test_code.c"):
            os.remove("test_code.c")
        if os.path.exists("test_program"):
            os.remove("test_program")

def verify_unit_test(source_code: str, vectorize_code_head: str, test_code: str) -> str:
    test_code = test_code + '\n'
    source_code_body = re.search(r'\{(.*)\}', source_code, re.DOTALL).group(0).strip()
    empty_body = '{}'
    
    pattern_to_match_return_type = r'^\s*(\w[\w\s\*\&\[\]]+)\s+\w+\s*\(.*\)\s*{.*}'
    match = re.match(pattern_to_match_return_type, source_code,re.DOTALL)
    if match:
        return_type = match.group(1).strip()
    else:
        print("#### match error")

    test_code1 = test_code +'\n'+ source_code + '\n' + vectorize_code_head + source_code_body
    test_output,_ = execute_test_code(test_code1)

    if 'PASS' not in test_output:
        return False

    if return_type == 'void':
        empty_body = '{}'
    else:
        pattern_to_match_return_ins = r'return\s+(.*?);'
        empty_body = re.sub(pattern_to_match_return_ins, r'return ~(\1);',source_code)

    test_code2 = test_code +'\n'+ source_code + '\n' + vectorize_code_head + empty_body
    test_output,_ = execute_test_code(test_code2)

    if 'PASS' in test_output:
        return False
    return True
    

cached_unit_test_code = None

def reset_cache():
    global cached_unit_test_code
    cached_unit_test_code = None

# Test semantic correctness and verify if the output is the same as the source code

def CorrectTest(input: str, source_code: str, vectorize_code: str, engine: str) -> str:
    # Generate a prompt for verifying the code
    prompt = f"""
Generate a unit test for the following C code to verify that the source code behaves consistently with the vectorized code. Randomly generate input.

Source Code:
```c
{source_code}
```

Vectorized Code:
```c
{vectorize_code}
```

Generate a unit test code that compares the output of the two pieces of code to see if the output is the same and wrap the results with ```c ```.
If the output is the same, print PASS. Only include the declaration of the original function and the vectorized function in the generated code and excluding specific implementations.
If comparing floating-point numbers, the allowed precision error is 1e-5.
"""
    global cached_unit_test_code
    retry_times = 10
    if cached_unit_test_code is None:
        for _ in range(retry_times):
            temp_cached_unit_test_code = generate_test_code(prompt, engine)
            # The first time, vectorize_code will only have the function declaration, so verify is specifically designed for this
            last_semicolon_index = vectorize_code.find(')')
            if last_semicolon_index != -1:
                vectorize_code_head = vectorize_code[:last_semicolon_index+1]

            if verify_unit_test(source_code, vectorize_code_head,temp_cached_unit_test_code):
                cached_unit_test_code = temp_cached_unit_test_code
                break
        if cached_unit_test_code is None:
            print("Error generate unit test")
            exit()
    
    test_code = cached_unit_test_code
        
    test_code = test_code +'\n'+ source_code + '\n' + vectorize_code
    print(test_code)

    # execute test code
    test_output,test_error = execute_test_code(test_code)

    return test_output, test_error

# test performance
def PerformanceTest(input: str, source_code: str, vectorize_code: str, engine:str) -> str:
    prompt = f"""
Generate a performance test for the following C code to compare the execution speed of the source code with that of the vectorized code.

Source Code:
```c
{source_code}
```

Vectorized Code:
```c
{vectorize_code}
```

Generate a test code that can measure and compare the performance of the two pieces of code. and wrap the generated code with ```c ```. Only include the declaration of the original function and the vectorized function in the generated code and excluding specific implementations.
"""

    test_code = generate_test_code(prompt, engine)
        
    if not test_code:
        return "Failed to generate test code."

    test_code = test_code +'\n'+ source_code + '\n' + vectorize_code
    print(test_code)
    # execute test code
    performance_output= execute_test_code(test_code)

    return performance_output

def test():
    feedback = CompilerTest('''int allPointsEq(float * pts, int count){
        for (int i = 0; i<count; i++){
            if (pts[0]!=pts[i]) return 0;
        }
        return 1;
    }
''')
    print(feedback)
    feedback2 = CompilerTest('''
    
#include <stdio.h>

int func() {
    int n = 1000;
    int a[n], b[n];
    for (int i = 0; i < n; ++i) {
        a[i] = i * 2;
    }
    for (int i = 0; i < n; ++i) {
        b[i] = a[i] + 3;
    }
    printf("%d\\n", b[10]);
    return 0;
}

    ''')
    print('###############################\n')
    print('###############################\n')
    print(feedback2)

    feedback3 = CompilerTest('''#include <arm_neon.h>

int allPointsEq_neon(float *pts, int count) {
    // Load the first element into a NEON register
    float32x4_t first_val = vld1q_dup_f32(&pts[0]);  // Duplicate the first element into all lanes

    int i;
    // Process the array in chunks of 4 elements (since NEON handles 4 floats at once)
    for (i = 0; i <= count - 4; i += 4) {
        // Load 4 floats from the array
        float32x4_t current_vals = vld1q_f32(&pts[i]);

        // Compare the loaded values with the first value
        uint32x4_t cmp_result = vceqq_f32(current_vals, first_val);

        // Check if all elements in the comparison result are non-zero (all equal)
        if (vminvq_u32(cmp_result) != 0xFFFFFFFF) {
            return 0;  // If any element is not equal, return 0
        }
    }

    // Handle any remaining elements if count is not a multiple of 4
    for (; i < count; i++) {
        if (pts[0] != pts[i]) {
            return 0;
        }
    }

    return 1;  // All elements are equal
}
 ''')
    print('###############################\n')
    print('###############################\n')
    print(feedback3)
    if feedback3=="":
        print("fine")


def testFormalVerify():
    source_code='''
void s1113(int iterations, int LEN_1D, float* a, float* b)
{
    for (int nl = 0; nl < 2*iterations; nl++) {
        for (int i = 0; i < LEN_1D; i++) {
            a[i] = a[LEN_1D/2] + b[i];
        }
    }
}
'''
    target_code='''
void s1113_opt(int iterations, int LEN_1D, float* a, float* b)
{
    for (int nl = 0; nl < 2 * iterations; nl++) {
        // Move the loop-invariant computation outside the inner loop
        float a_half = a[LEN_1D / 2];

        // Vectorize the inner loop
        for (int i = 0; i < LEN_1D; i++) {
            a[i] = a_half + b[i];
        }
    }
}
'''
    result = FormalVerification(source_code, target_code)
    print(result)


if __name__ == "__main__":
    testFormalVerify()
