cmake_minimum_required(VERSION 2.8)

project(ONNXRunner)

set(CMAKE_CXX_STANDARD 17)

set(INC_DIR /usr/include)
set(INC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include) 
set(LIB_DIR /usr/lib64)
 
include_directories(
	${INC_HEADER}
	${INC_DIR})
 
link_directories(${LIB_DIR})

add_library(ONNXRunner SHARED ONNXRunner.cpp)

target_link_libraries(ONNXRunner
PRIVATE
libcrypto.so) # libonnxruntime.so
