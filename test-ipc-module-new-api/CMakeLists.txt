cmake_minimum_required(VERSION 3.14)
project(TestIpcNewApiPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Universal module: `interface: "universal"` in metadata.json makes the builder
# run `logos-cpp-generator --from-header` in preConfigure, emitting the Qt
# plugin glue, the dispatch and the typed event emitters into generated_code/ —
# LogosModule.cmake picks those up automatically.
logos_module(
    NAME test_ipc_new_api_module
    SOURCES
        src/test_ipc_new_api_impl.h
        src/test_ipc_new_api_impl.cpp
)
