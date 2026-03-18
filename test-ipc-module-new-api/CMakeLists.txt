cmake_minimum_required(VERSION 3.14)
project(TestIpcNewApiPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

logos_module(
    NAME test_ipc_new_api_module
    NATIVE_PROVIDER_HEADER src/test_ipc_new_api_impl.h
    SOURCES
        src/test_ipc_new_api_loader.h
        src/test_ipc_new_api_impl.h
        src/test_ipc_new_api_impl.cpp
        logos_provider_dispatch.cpp
)
