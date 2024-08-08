cmake_minimum_required(VERSION 2.8)

project(ONNXRunner)

set(CMAKE_CXX_STANDARD 17)

#-------------------------------------------------------------------------------
# Dependency settings
#-------------------------------------------------------------------------------
# Set include file directory for AI4Compiler framework.
set(FRAMEWORK_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include")

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

# Set directory for dependency libraries.
set(DEFAULT_LIB_DEP_DIR "/usr/lib64" CACHE FILEPATH
    "Default directory for dependency libraries")

# Search for dependency library libcrypto.so.
if(NOT DEFINED crypto_LIBDIR)
  message("Set default path to search libcrypto as ${DEFAULT_LIB_DEP_DIR}. "
          "Use -Dcrypto_LIBDIR to change lib path.")
  set(crypto_LIBDIR ${DEFAULT_LIB_DEP_DIR})
endif()

find_library(LIBCRYPTO
             NAMES crypto libcrypto
             PATHS "${crypto_LIBDIR}")
if(NOT LIBCRYPTO)
  message(FATAL_ERROR "libcrypto library is not found! ")
endif()

add_library(libcrypto SHARED IMPORTED)
set_target_properties(libcrypto PROPERTIES
                      IMPORTED_LOCATION "${LIBCRYPTO}")

#-------------------------------------------------------------------------------
# Framework Compilation and Installation
#-------------------------------------------------------------------------------
# Create a dynamic library for AI4Compiler framework.
set(ai4compiler ${PROJECT_NAME})
add_library(${ai4compiler} SHARED ONNXRunner.cpp)

target_include_directories(${ai4compiler}
                           PRIVATE
                           ${FRAMEWORK_INCLUDE}
                           "${onnxruntime_ROOTDIR}/include"
                           "${onnxruntime_ROOTDIR}/include/onnxruntime")

target_link_libraries(${ai4compiler}
                      PRIVATE
                      libcrypto)

# Install the targets and include files to expected locations.
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(CMAKE_INSTALL_LIBDIR "./lib")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CMAKE_INSTALL_LIBDIR "./lib64")
  else()
    message(WARNING "Unknown system architecture. Defaulting to './lib'.")
    set(CMAKE_INSTALL_LIBDIR "./lib")
  endif()
  message(STATUS "Setting CMAKE_INSTALL_LIBDIR to '${CMAKE_INSTALL_LIBDIR}'.")
endif()

if(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
  set(CMAKE_INSTALL_INCLUDEDIR "./include")
  message(STATUS
          "Setting CMAKE_INSTALL_INCLUDEDIR to "
          "'${CMAKE_INSTALL_INCLUDEDIR}'.")
endif()

install(TARGETS ${ai4compiler}
        DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY "${FRAMEWORK_INCLUDE}/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        FILES_MATCHING
        PATTERN "*.h")

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../models/"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/AI4C"
        FILES_MATCHING
        PATTERN "*.onnx")
