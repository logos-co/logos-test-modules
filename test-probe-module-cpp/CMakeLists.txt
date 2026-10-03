cmake_minimum_required(VERSION 3.14)
project(TestProbeModuleCppPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Universal, on qt_remote_plain: a native module, so the runtime can hand it a
# configuration and scope the tokens it is called with.
logos_module(
    NAME test_probe_module_cpp
    SOURCES
        src/test_probe_module_cpp_impl.h
        src/test_probe_module_cpp_impl.cpp
)
