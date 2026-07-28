cmake_minimum_required(VERSION 3.14)
project(TestFullapiExtCppPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Universal module: `interface: "universal"` makes the builder derive the
# contract from the impl header — records included — and emit the Qt plugin
# glue, the dispatch and the codec into generated_code/.
#
# SOURCES is not optional for a C++ module: without it the impl translation
# unit is never compiled, the plugin links with the impl's symbols undefined,
# and the module SIGSEGVs on the first call rather than failing to build.
logos_module(
    NAME test_fullapi_ext_cpp
    SOURCES
        src/test_fullapi_ext_cpp_impl.h
        src/test_fullapi_ext_cpp_impl.cpp
)
