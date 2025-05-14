import sys
from typing import Dict, List
from src.utils import Prompt
import pandas as pd
import re

from prompt_lib.backends import openai_api


class SplitGenTaskIterate(Prompt):
    def __init__(self, engine: str, prompt_examples: str) -> None:
        super().__init__(
            question_prefix="",
            answer_prefix="Vectorized code: ",
            intra_example_sep="\n\n",
            inter_example_sep="\n\n###\n\n",
        )
        self.engine = engine
        self.count = 0
        self.prompt = self.make_prompt(prompt_examples=prompt_examples)

    def make_prompt(self, prompt_examples: str) -> str:
        instr = '''I want to vectorize the following loop,
could you reorganize the loop structure so that compiler can better vectorize it? 
don't vectorize it, just reorganize the loop. 
if there is loop-carried dependency , 
you can consider Loop splitting, Loop reordering, Instruction reordering, Loop distribution and other technicues to separate the dependency out.
Pay attention to the dependency between loops.
Choose a more simplified solution.
'''
        return instr

    def make_query(self, question: str) -> str:
        return f"{self.prompt}{self.question_prefix}{question}{self.intra_example_sep}"

    def _make_input(
        self,
        code: str,
        vectorize_code: str,
        llm_feedback: str,
        compiler_feedback: str,
        unit_test_feedback:str,
    ) -> str:
        input_txt = f"""Source code: 
```c
{code}
```

Optimized code:
```c
{vectorize_code}
```

Feedback:
{llm_feedback}

Compiler Feedback:
{compiler_feedback}

UnitTest Feedback:
{unit_test_feedback}

**Objective**: Generate a semantically equivalent vectorize-friendly version of `*_opt` using:  
**Preservation Requirements**:  
1. Function signature

If the original value is used, e.g. a[i+1] is used for iteration i, a[i+1] should be stored in a temporary variable(use memcpy to store orign value) or used before updating a[i+1]
If the updated value is used, e.g. a[i-1] is used for iteration i, a[i-1] should update before using it.

**Possible Transformations**  
Loop Splitting:  
**Type 1 - Instruction Splitting**  
   *When to use*:  
   - Multiple independent statements exist within the loop body  
   - Example pattern:  
     ```c
     for(i) {{ 
       A[i] = ...;  // Independent group 1
       B[i] = ...;  // Independent group 2 
     }}
     ```  
   *Implementation*:  
   ```c
   for(i) {{ A[i] = ...; }}  // Split loop 1  
   for(i) {{ B[i] = ...; }}  // Split loop 2  
   ```  
   *Validation*:  
   - Total iteration count remains identical  
   - Memory access order preserved across all arrays  

**Type 2 - Iteration Range Splitting**  
   *When to use*:  
   - Loop contains phase-dependent patterns (e.g., mid-point update)  
   - Vectorization blocked by conditional value changes  
   - Example pattern:  
     ```c
     for(i=0; i<LEN; i++) a[i] = a[mid] + b[i];  
     ```  
   *Implementation*:  
   ```c
   // Phase 1: i=0~mid (pre-update)  
   for(i=0; i<mid; i++) a[i] = a[mid] + b[i];  
   // Mid-point update  
   a[mid] = a[mid] + b[mid];  
   // Phase 2: i=mid+1~end (post-update)  
   for(i=mid+1; i<LEN; i++) a[i] = a[mid] + b[i];  
   ```  
   *Validation Checklist*:  
   - Split boundaries cover original iteration range  
   - No overlapping writes between split segments  

Loop Reordering and Instruction Reordering:
**Application Rules**:  
   *Priority Patterns*:  
   - Move loop-invariant computations upward  
     ```c
     // Before reorder:
     a[i] = x + y;  // y is invariant
     b[i] = y * z;
     
     // After reorder:
     b[i] = y * z;  // y used earlier
     a[i] = x + y;
     ```  
   - Solve loop dependency
    ```c
    // Before reorder:
     a[i] = b[i-1] + y;  // y is invariant
     b[i] = y * z;
     
     // After reorder:
     b[i] = y * z;  // y used earlier
     a[i] = b[i-1] + y;
     // Before reorder:
     a[i] = b[i-1] + y;  // y is invariant
     b[i] = a[i+1] * z;
     
     // After reorder:
     b[i] = a[i+1] * z;
     a[i] = b[i-1] + y;
     ```
   - Cluster memory operations with similar addresses 

Loop Distribution:
**When to Apply**:  
   - Multiple independent write targets in loop body  
   - Example:  
     ```c
     // Original
     for(i) {{
       A[i] = ...;  // Writes to A
       B[i] = ...;  // Writes to B (independent)
     }}
     
     // Distributed
     for(i) {{ A[i] = ...; }}  
     for(i) {{ B[i] = ...; }}  
     ```  
   *Validation*:  
   - Distributed loops must have identical iteration ranges  
   - No interleaved access between distributed loops  

Use temporary variable:
Use temp variable to store result to avoid complex memory access

Remove branching logic (if, else if, switch) from loops to enable automatic vectorization:
Replace multi-way conditionals with arithmetic masking, ensuring uniform execution paths across iterations.
**When to Apply**:  
    - Example:  
     ```c
    // Original
    if (cond1)  
        x = value_A;  
    else if (cond2)  
        x = value_B;  
    else  
        x = value_C;
    // Optimized  
    x = cond1 * value_A + (cond2 && !cond1) * value_B + (!cond1 && !cond2) * value_C;  
     ```

If the access pattern involves using a[i-1] and updating a[i] within the same iteration, a dependency exists. Attempt to reorder the instructions to resolve this dependency.

Improve the optimized code based on the preceding techniques and other optimization methods.

Okay, let's use this feedback to refactor the code and encapsulate the generated code with ``c ``. Do not change the function head tailed with _opt. Only generate one optimized code. Do not output other code.Do not output orign code.

"""

        return input_txt

    def __call__(
        self,
        code,
        vectorize_code,
        llm_feedback,
        compiler_feedback,
        unit_test_feedback,
        outputFileName
    ) -> str:
        example_input = self.make_input(
            code=code,
            vectorize_code=vectorize_code,
            llm_feedback=llm_feedback,
            compiler_feedback = compiler_feedback,
            unit_test_feedback =unit_test_feedback,
        )
        transfer_query = self.make_query(example_input)
        self.count += 1
        
        print("##################### ITERATE PROMPT ######################")
        print(transfer_query)
        print("##################### ITERATE PROMPT ######################")
        with open(outputFileName, "a") as of:
            of.write(f"## ITERATE PROMPT\n")
            of.write(transfer_query)
            of.write("\n\n")

        transfer_query = [{"role":"user", "content":transfer_query}]
        output = openai_api.OpenaiAPIWrapper.call(
            prompt=transfer_query,
            engine=self.engine,
            max_tokens=4096,
            stop_token=None,
            temperature=1.0,
        )
        
        response = openai_api.OpenaiAPIWrapper.get_first_response(output)


          
        print("##################### ITERATE OUTPUT ######################")
        if "</think>" in response:
            print(response.split("</think>", 1)[0])
            print("</think>")
            response = response.split("</think>", 1)[1]
        print(response)
        print("##################### ITERATE OUTPUT ######################")

        with open(outputFileName, "a") as of:
            of.write(f"## ITERATE OUTPUT\n")
            of.write(response)

        pattern = r'```c(.*?)```'
        response = re.findall(pattern, response, re.DOTALL)[0]
  
        return response

    # TODO:这里只考虑了前一轮的Feedback，实际上应该考虑前面所有轮次的
    def make_input(
        self,
        code,
        vectorize_code,
        llm_feedback,
        compiler_feedback,
        unit_test_feedback,
    ) -> str:
        input_txt = ""
        input_txt += self._make_input(
            code=code,
            vectorize_code=vectorize_code,
            llm_feedback=llm_feedback,
            compiler_feedback=compiler_feedback,
            unit_test_feedback=unit_test_feedback,
        )
        return input_txt
    

if __name__ == "__main__":
    obj = SplitGenTaskIterate(prompt_examples="data/prompt/vectorize/feedback.jsonl", engine="starlight/DeepSeek-R1-671B")
    print(obj.prompt)

    code='''void s115(int iterations, float* a, float aa[256][256])
{
    for (int nl = 0; nl < 1000*(iterations/256); nl++) {
        for (int j = 0; j < 256; j++) {
            for (int i = j+1; i < 256; i++) {
                a[i] -= aa[j][i] * a[j];
            }
        }
    }
}'''

    vectorize_code='''void s115_opt(int iterations, float* a, float aa[256][256])
{
    for (int nl = 0; nl < 1000*(iterations/256); nl++) {
        for (int j = 0; j < 256; j++) {
            float temp = a[j];
            for (int i = j+1; i < 256; i++) {
                a[i] -= aa[j][i] * temp;
            }
        }
    }
}
'''
    llm_feedback='''The code cannot be automatically vectorized due to the following issues:

1. **Conditional Dependencies**: The loop contains conditional statements (`if (srcFixed <= dstLen)`) that depend on the loop index, which prevents the compiler from vectorizing the loop.

2. **Data Dependencies**: The loop updates `isScalable` and uses its value in subsequent iterations, creating a data dependency that prevents vectorization.

3. **Indirect Indexing**: The loop uses indirect indexing (`src[i + 1] = divs[i]`), which can complicate vectorization.

### Suggestions for Refactoring:

1. **Eliminate Conditional Dependencies**: Refactor the loop to avoid conditional statements that depend on the loop index. You can precompute the values that depend on the loop index and store them in an array before the loop.

2. **Remove Data Dependencies**: Separate the logic for "scalable" and "fixed" patches into two separate loops or precompute the values for each case before entering the loop.

3. **Simplify Indirect Indexing**: If possible, replace indirect indexing with direct indexing or precompute the indices before the loop.

4. **Unroll Small Loops**: If `divCount` is small, consider unrolling the loop manually to allow for better optimization by the compiler.

5. **Use Compiler Directives**: If the compiler supports it, use pragmas or attributes to hint at vectorization opportunities, such as `#pragma omp simd` or `__attribute__((vectorize))`.

By addressing these issues, you can make the code more amenable to automatic vectorization while maintaining its original semantics.
 
'''

    temp = obj(code=code, vectorize_code=vectorize_code, llm_feedback=llm_feedback, outputFileName="./log/temp.txt")