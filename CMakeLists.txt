cmake_minimum_required(VERSION 3.22)

project(ai4c)

set(CMAKE_CXX_STANDARD 17)

option(BUILD_ONNXRUNNER "Build onnxrunner" ON)
option(BUILD_ONNXRUNNER_LITE "Build onnxrunner_lite" OFF)
option(BUILD_GCC_PLUGINS "Build gcc-plugins" ON)

message(STATUS "Build onnxruner: " ${BUILD_ONNXRUNNER})
message(STATUS "Build onnxruner lite: " ${BUILD_ONNXRUNNER_LITE})
message(STATUS "Build gcc-plugins: " ${BUILD_GCC_PLUGINS})

#-------------------------------------------------------------------------------
# Dependency settings
#-------------------------------------------------------------------------------
# Set include file directory for AI4Compiler framework.
set(FRAMEWORK_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/aiframe/include")

# Set root directory for ONNXRuntime library.
set(DEFAULT_onnxruntime_ROOTDIR "/usr" CACHE FILEPATH
    "Default root directory for ONNXRuntime")

if(NOT DEFINED onnxruntime_ROOTDIR)
  message(WARNING
          "Set default root path to libonnxruntime as "
          "${DEFAULT_onnxruntime_ROOTDIR}. "
          "Use -Donnxruntime_ROOTDIR to change ONNXRuntime root path.")
  set(onnxruntime_ROOTDIR ${DEFAULT_onnxruntime_ROOTDIR})
endif()

list(APPEND FRAMEWORK_INCLUDE 
            "${onnxruntime_ROOTDIR}/include"
            "${onnxruntime_ROOTDIR}/include/onnxruntime"
            "${onnxruntime_ROOTDIR}/include/onnxruntime/core/session")

set(ONNXRUNTIME_LIB "${onnxruntime_ROOTDIR}/lib64")
message(STATUS "ONNXRuntime: ${ONNXRUNTIME_LIB}")

add_subdirectory(aiframe)
add_subdirectory(aiframe_legacy)
