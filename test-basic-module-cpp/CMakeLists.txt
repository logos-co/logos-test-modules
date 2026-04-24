cmake_minimum_required(VERSION 3.14)
project(TestBasicModuleCppPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Universal module: `interface: "universal"` in metadata.json makes the
# builder run `logos-cpp-generator --from-header` in preConfigure. The
# generator produces `generated_code/test_basic_module_cpp_qt_glue.h` +
# `generated_code/test_basic_module_cpp_dispatch.cpp` which LogosModule.cmake
# picks up automatically — no loader / dispatch source needs to live here.
logos_module(
    NAME test_basic_module_cpp
    SOURCES
        src/test_basic_module_cpp_impl.h
        src/test_basic_module_cpp_impl.cpp
)
