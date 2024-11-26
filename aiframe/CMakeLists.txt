function(create_onnxrunner lib_name src_file)
  add_library(${lib_name} SHARED ${src_file})

  set_target_properties(${lib_name} PROPERTIES PREFIX "")
  target_include_directories(${lib_name} PRIVATE ${FRAMEWORK_INCLUDE})

  target_link_libraries(${lib_name}
                        PRIVATE
                        ${ONNXRUNTIME_LIB})

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

endfunction()

#-------------------------------------------------------------------------------
# Create a dynamic library for AI4Compiler framework
#-------------------------------------------------------------------------------
if (BUILD_ONNXRUNNER)
  create_onnxrunner(ai4c_onnxrunner "onnxrunner.cpp")
endif()

if (BUILD_ONNXRUNNER_LITE)
  message(WARNING "ai4c_onnxrunner_lite is not ready yet.")
endif()

if (BUILD_GCC_PLUGINS)
  add_subdirectory(gcc_plugins)
endif()