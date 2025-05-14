import pandas as pd
import re
from src.utils import Prompt

from prompt_lib.backends import openai_api


class SplitGenTaskInit(Prompt):
    def __init__(self, prompt_examples: str, engine: str) -> None:
        super().__init__(
            question_prefix="Source Code: ",
            answer_prefix="Vectorized code: ",
            intra_example_sep="\n\n",
            inter_example_sep="\n\n---\n\n",
        )
        self.engine = engine
        self.prompt = '''
Add the suffix _opt to the function name and encapsulate the code with ``c ``. Only the declaration is output, not the definition.
'''
    
    def make_query(self, code: str, compiler_feedback: str) -> str:
        query = f'''{self.prompt}\n
```c
{code}
```
'''
        return query

    def __call__(self, code: str, compiler_feedback:str, outputFileName:str) -> str:
        generation_query = self.make_query(code, compiler_feedback)
        print("####################### INIT PROMPT #######################")
        print(generation_query)
        print("####################### INIT PROMPT #######################")
        with open(outputFileName, "a") as of:
            of.write(f"## INIT PROMPT\n")
            of.write(generation_query)

        generation_query = [{"role":"user", "content":generation_query}]


        output = openai_api.OpenaiAPIWrapper.call(
            prompt=generation_query,
            engine=self.engine,
            max_tokens=4096 ,
            stop_token=None,
            temperature=0,
        )
        generated_vec = openai_api.OpenaiAPIWrapper.get_first_response(output)
        print("####################### INIT OUTPUT #######################")
        if "</think>" in generated_vec:
            print(generated_vec.split("</think>", 1)[0])
            print("</think>")
            generated_vec = generated_vec.split("</think>", 1)[1]
        print(generated_vec)
        print("####################### INIT OUTPUT #######################")
        with open(outputFileName, "a") as of:
            of.write(f"## INIT OUTPUT\n")
            of.write(generated_vec)
        pattern = r'```c(.*?)```'
        generated_vec = re.findall(pattern, generated_vec, re.DOTALL)[0]
        return generated_vec.strip()


def test():
    task_init = SplitGenTaskInit(engine="deepseek-reasoner", prompt_examples="data/prompt/vectorize/init.jsonl")
    print(task_init.prompt)
    vectorizedCode = task_init('''void set_points(float* dst, int* src, const int* divs, int divCount, int srcFixed,
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
''', "","./log/temp.txt")


if __name__ == "__main__":
    test()
