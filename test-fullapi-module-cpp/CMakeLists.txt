cmake_minimum_required(VERSION 3.14)
project(TestFullapiCppPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Universal module: `interface: "universal"` in metadata.json makes the builder
# run the code generator over the impl header in preConfigure, emitting the Qt
# plugin glue + dispatch + typed-event sources into generated_code/, which
# LogosModule.cmake globs automatically.
logos_module(
    NAME test_fullapi_cpp
    SOURCES
        src/test_fullapi_cpp_impl.h
        src/test_fullapi_cpp_impl.cpp
)
