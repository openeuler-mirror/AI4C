import pandas as pd
import sys
from prompt_lib.backends import openai_api

from src.utils import Prompt


class SplitGenFeedback(Prompt):
    def __init__(self, engine: str, prompt_examples: str, max_tokens: int = 4096) -> None:
        super().__init__(
            question_prefix="Source Code: ",
            answer_prefix="Vectorized code: ",
            intra_example_sep="\n\n",
            inter_example_sep="\n\n###\n\n",
        )
        self.engine = engine
        self.max_tokens = max_tokens
        self.setup_prompt_from_examples_file(prompt_examples)

    def setup_prompt_from_examples_file(self, examples_path: str) -> str:
        instruction = '''
1. Check for Compilation Errors in Optimized Code:
- First, check if the Optimized Code compiles successfully.
- If there are compilation errors in the Optimized Code:
  - Do not proceed to the next stages (semantic comparison, vectorization, or code improvement). And Output FAIL.
  - Provide specific fixes to resolve the compilation errors in the Optimized Code.
  - Only proceed with the analysis after resolving compilation errors in the Optimized Code.
2. Semantic Comparison with Simple Test Cases:
- If the Optimized Code compiles without errors, proceed to compare the semantics of the Optimized Code with the Source Code.
- Key checks:
  - Check unit Test analysis output, if it indicates that the output is inconsistent, it indicates that the semantics is wrong.
  - Dependencies: Identify true dependencies (read-after-write) and false dependencies (read-before-write). Ensure that the order and usage of variables (e.g., a[i], a[i + 1], b[i], etc.) in the Optimized Code do not introduce unintended side effects or alter the semantics.
- Please execute three examples step by step to verify your judgment.
- If the semantics in the Optimized Code differ from the Source Code, provide suggestions for modifying the Optimized Code so that it matches the Source Code's semantics. Stop the process here and output FAIL if semantics are incorrect.
3. Vectorization Analysis (Optimized Code Only):
- If the Optimized Code's semantics match the Source Code, proceed to analyze whether the Optimized Code can be automatically vectorized.
- Check compiler feedback fields: If not output loop not vectorized, the vectorization is considered successful. Skip the subsequent phases and directly output PASS.
- Key factors to consider:
  - Loop-carried dependencies: Identify any true dependencies (read-after-write) that would prevent vectorization.
  - False dependencies: Identify read-before-write dependencies and check if they can be eliminated to enable vectorization.
- If the Optimized Code is vectorizable, proceed to suggest improvements for further optimizations.
  - Loop optimization: Reorganize the loop structure to improve vectorization.
  - Memory access patterns: Improve memory access to optimize cache locality and reduce latency.
  - Eliminate unnecessary dependencies: Reduce or eliminate any dependencies that hinder parallelism or vectorization.
- If vectorization is not possible, provide a detailed analysis of why automatic vectorization is not possible in the Optimized Code.
  - Suggest potential changes to the Optimized Code to make it vectorization-friendly.
  - Optimization: Loop splitting, Loop reordering, Instruction reordering, Loop distribution and so on.
  - If there is optimization opportunity, output FAIL.
4. Only output PASS if the code is fully optimized, with no further modification needed.
5. Output analysis only and do not output any code.
'''
    #   - If the loop is not fully vectorizable, the compiler can try to apply loop unrolling followed by the SLP vectorizer in order to partially vectorize the loop.

        self.prompt = instruction
        
    
    def __call__(self, code: str, vectorize_code: str, compiler_feedback: str, outputFileName:str, unit_test_feedback:str):
        prompt = self.get_prompt_with_question(code=code, vectorize_code=vectorize_code, compiler_feedback=compiler_feedback, unit_test_feedback=unit_test_feedback)
        print("##################### FEEDBACK PROMPT #####################")
        print(prompt)
        print("##################### FEEDBACK PROMPT #####################")
        with open(outputFileName, "a") as of:
            of.write(f"## FEEDBACK PROMPT\n")
            of.write(prompt)
        prompt = [{"role":"user", "content":prompt}]
        output = openai_api.OpenaiAPIWrapper.call(
            prompt=prompt,
            engine=self.engine,
            max_tokens=self.max_tokens,
            stop_token=None,
            temperature=1.0,
        )
        generated_feedback = openai_api.OpenaiAPIWrapper.get_first_response(output)
        print("##################### FEEDBACK OUTPUT #####################")
        if "</think>" in generated_feedback:
            print(generated_feedback.split("</think>", 1)[0])
            print("</think>")
            generated_feedback = generated_feedback.split("</think>", 1)[1]
        print(generated_feedback)
        print("##################### FEEDBACK OUTPUT #####################")
        with open(outputFileName, "a") as of:
            of.write("## FEEDBACK OUTPUT\n")
            of.write(generated_feedback)
        return generated_feedback

    def get_prompt_with_question(self, code: str, vectorize_code: str, compiler_feedback: str, unit_test_feedback:str):
        question = self.make_query(code=code, vectorize_code=vectorize_code, compiler_feedback=compiler_feedback)
        return f"""{question}{unit_test_feedback}{self.prompt}"""

    def make_query(self, code: str, vectorize_code: str, compiler_feedback: str):
        question = f"""Source code: 
```c
{code}
```

Optimize code:
```c
{vectorize_code}
```

Compiler feedback: {compiler_feedback}
"""

        return question



if __name__ == "__main__":
    feedback = SplitGenFeedback(
        engine="deepseek-chat",
        prompt_examples="data/prompt/vectorize/feedback.jsonl",
    )
    code='''void add_arrays_non_vectorized(float *a, float *b, float *result, size_t size) {
    for (size_t i = 0; i < size; i++) {
        result[i] = a[i] + b[i];                                            // Add corresponding elements of a and b
    }
}'''

    vectorize_code='''void set_points(float* dst, int* src, const int* divs, int divCount, int srcFixed,
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

    feedback(code=code, vectorize_code=vectorize_code, compiler_feedback='', outputFileName = "./log/temp.txt")
    
    print(feedback.prompt)
