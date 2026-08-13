cmake_minimum_required(VERSION 3.14)
project(DummyModule000000Plugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Universal module: `interface: "universal"` in metadata.json makes the builder
# run `logos-cpp-generator --from-header` in preConfigure, which emits the Qt
# plugin glue and dispatch into generated_code/ — LogosModule.cmake picks those
# up automatically, so no loader or dispatch source is listed here.
logos_module(
    NAME dummy_module_000000
    SOURCES
        src/dummy_module_000000_impl.h
        src/dummy_module_000000_impl.cpp
)
