function(add_gcc_plugin target_name)
  set(SOURCE_FILES ${ARGN})
  set(lib_name ${target_name}_gcc${GCC_VERSION})
  add_library(${lib_name} SHARED ${SOURCE_FILES})
  set_target_properties(${lib_name} PROPERTIES PREFIX "")
  target_include_directories(${lib_name} PRIVATE
                             ${INCLUDE_DIRS} 
                             ${FRAMEWORK_INCLUDE})
  target_link_libraries(${lib_name} yaml-cpp)
endfunction()

if("${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")

  get_filename_component(GCC_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
  execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=plugin
      OUTPUT_VARIABLE GCC_PLUGIN_PATH
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  file(GLOB INCLUDE_DIRS ${GCC_PLUGIN_PATH}/include)
  execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
      OUTPUT_VARIABLE GCC_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REPLACE "." ";" VERSION_LIST ${GCC_VERSION})
  list(GET VERSION_LIST 0 GCC_VERSION)

  add_subdirectory(autotune)
  add_subdirectory(optimize)
endif()