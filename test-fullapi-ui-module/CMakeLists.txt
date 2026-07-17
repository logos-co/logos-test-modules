cmake_minimum_required(VERSION 3.14)
project(TestFullapiUiPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found. Set LOGOS_MODULE_BUILDER_ROOT.")
endif()

# Derive the module name from metadata.json — single source of truth.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/metadata.json" METADATA_JSON)
string(JSON MODULE_NAME GET ${METADATA_JSON} name)

# Universal UI module with an interface dependency: REP_FILE runs repc; the
# generated *Plugin glue + the Qt-typed bind_full_api(name) wrapper (from
# metadata.json#interface_dependencies) land in generated_code/ and compile
# automatically.
logos_module(
    NAME ${MODULE_NAME}
    REP_FILE src/test_fullapi_ui.rep
    SOURCES
        src/test_fullapi_ui_backend.h
        src/test_fullapi_ui_backend.cpp
    INCLUDE_DIRS
        src
)
