# LLM4Compiler - G2CTrans: code lifting agent for translating CUDA kernel on GPU to C/C++ kernel on CPU.

The developed G2CTrans agent is based on LangChain. Two kinds of feedback are implemented: 
(1) semantic correctness information from automatic testing-based method;
(2) LLM itself(self-feedback); 

and in the future, we will add new feedback from
(3) formal verification for complicated code and performance from the runtime phases. -- TODO

The G2CTrans agent can be combined with VecTrans agent to achieve high-performance translation across different plateforms.

To use G2CTrans, follow the steps:

### (1) dependencies: ```LangChain```, ```LangGraph```
```bash
pip install langchain
pip install langgraph
```

### (2) run the iterative inference
> 
The function entrance is ```./CUDA_C_Agent.py```. Run the a command:
```
python CUDA_C_Agent.py
```
