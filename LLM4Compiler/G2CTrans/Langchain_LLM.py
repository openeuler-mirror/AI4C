from langchain.prompts import PromptTemplate
from langchain_openai import OpenAI
from langchain_core.runnables import RunnableLambda, RunnablePassthrough
from langchain_core.output_parsers import StrOutputParser
import re
import os
import httpx
import Global_vars as GV
import asyncio

http_client = httpx.Client(verify=False)

# 1. input processer
class PythonInputProcessor:
    def transform(self, code: str) -> str:
        """standardize input code"""
        # add preprocessing module(like remove annotation/blank）
        #print(f"code: {code}")
        #print("===========================")
        clean_code = "\n".join([line for line in code.splitlines() if line.strip()])
        #print(f"# Input Type: CUDA\n{clean_code}")
        #print("===========================")
        return f"# Input Type: CUDA\n{clean_code}"

# 2. output parser
class CodeExtractor:
    def extract(self, text: str) -> str:
        """abstract pure CPP code from LLM output"""
        print("=============================")
        print(f"text: {text}")
        pattern = r"```cpp\n(.*?)```"
        match = re.search(pattern, text, re.DOTALL)
        print("=============================")
        print(f"Intermediate Code: {match.group(1).strip()}")
        return match.group(1).strip() if match else text

# update propmpt
class unittest:
    def updateprompt(self, text: str):
        #
        filename = GV.ABS_SAVE_PATH + str(GV.FILE_INDEX) + "_cpp_opt.cpp"
        #
        if os.path.exists(filename) and os.path.isfile(filename):
            os.remove(filename)
        with open(filename, "a") as f:
            f.write(f"// Optimized code(id = {GV.FILE_INDEX}): \n\n{text}")
        #
        OPTIMIZATION_PROMPT = PromptTemplate.from_template("""
        You have assigned a task to generate a main function and test cases. The requirements include:
        1. analyze the given function of cpp source code;
        2. generate the test cases including main function to confirm it compilable without any errors;
        3. the given cpp function is:
        ```cpp
        {text}
        ```
        4. the final unit test source code must be fully C++ code with main function and without any explanatory samples.
        """)
        return OPTIMIZATION_PROMPT

# 3. build complete chain-workflow
def build_tritransform_agent(sys_prompt: str = None):
    # prompt tempalte
    if sys_prompt != None:
        OPTIMIZATION_PROMPT = PromptTemplate.from_template(sys_prompt)
    else:
        OPTIMIZATION_PROMPT = PromptTemplate.from_template("""
        You are a code translation engineer tasked with translating CUDA kernel function code into high-level serial C++ code that can be run on a CPU. The translated code must have correct syntax and produce results identical to the original CUDA code. Analyze the structure of the following CUDA code and complete the CUDA-to-C++ translation task:
        1. analyze the syntax and semantics of the original CUDA code;
        2. generate the corresponding semantic-equivalent sequential C++ source code;
        3. check the generated C++ source code for the syntax and semantic correctness, if not correct, try to repair;
        4. the final optimized LLM output must be fully C++ code without any other explanatory samples.

        original code：
        ```cuda
        {code}
        ```

        optimized code：
        ```cpp

        ```
        """)

    ENGINE = os.getenv("ENGINE")
    # model configurations
    llm = OpenAI(
        model=ENGINE,
        temperature=0.1,
        max_tokens=8192,
        http_client=http_client
    )

    # build processing components
    input_processor = RunnableLambda(PythonInputProcessor().transform)
    output_extractor = RunnableLambda(CodeExtractor().extract)

    unittest_generator = RunnableLambda(unittest().updateprompt)
    
    # build processing chains
    #'''
    chain = (
        {"code": input_processor}
        | OPTIMIZATION_PROMPT
        | llm
        | StrOutputParser()
        | output_extractor
        | unittest_generator
        | llm
        | StrOutputParser()
        | output_extractor
    )
    #'''
    '''
    chain = (
        {"code": input_processor}
        | OPTIMIZATION_PROMPT
        | llm
        | StrOutputParser()
        | output_extractor
    )
    '''

    return chain

# 4. Agent encapsulation
class TriTransformAgent:
    def __init__(self, sys_prompt: str = None):
        self.chain = build_tritransform_agent(sys_prompt)

    async def stream_response(self, input_code: str = None):
        res = ""
        async for chunk in self.chain.astream(input_code):
            print(chunk, end="|", flush=True)
            res += chunk
        return res

    def run(self, input_code: str) -> str:
        #return asyncio.run(self.stream_response(input_code))
        return self.chain.invoke(input_code)

# 5. usecases
if __name__ == "__main__":
    # CUDA-C: system prompt
    agent = TriTransformAgent()

    test_code = """
    __global__ void add_sources_d ( const float * const model , float * wfp , const float * const source_amplitude , const int * const sources_z , const int * const sources_x , const int nz , const int nx , const int nt , const int ns , const int it ) { int x = threadIdx . x ; int b = blockIdx . x ; int i = sources_z [ b * ns + x ] * nx + sources_x [ b * ns + x ] ; int ib = b * nz * nx + i ; wfp [ ib ] += source_amplitude [ b * ns * nt + x * nt + it ] * model [ i ] ; } 
    """
    #
    optimized_code = agent.run(test_code)
    print(f"Original Code: \n{test_code} \n   \nOptimized Code：\n{optimized_code}")
    #
    filename = "opt_code.cpp"
    if os.path.exists(filename) and os.path.isfile(filename):
        os.remove(filename)
    with open(filename, "a") as f:
        f.write(f"// original code: \n /*\n{test_code}\n*/\n")
        f.write(optimized_code)
