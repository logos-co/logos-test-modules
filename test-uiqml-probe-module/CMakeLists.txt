cmake_minimum_required(VERSION 3.14)
project(TestUiqmlProbePlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found. Set LOGOS_MODULE_BUILDER_ROOT.")
endif()

# Derive the module name from metadata.json — single source of truth.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/metadata.json" METADATA_JSON)
string(JSON MODULE_NAME GET ${METADATA_JSON} name)

# Universal UI module: REP_FILE runs repc; the generated *Plugin glue and the
# Qt-typed test_fullapi_cpp wrapper land in generated_code/ and compile
# automatically.
logos_module(
    NAME ${MODULE_NAME}
    REP_FILE src/test_uiqml_probe.rep
    SOURCES
        src/test_uiqml_probe_backend.h
        src/test_uiqml_probe_backend.cpp
    INCLUDE_DIRS
        src
)
