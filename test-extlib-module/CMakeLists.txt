cmake_minimum_required(VERSION 3.14)
project(TestExtlibModulePlugin LANGUAGES CXX C)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Build the external C library as a static library
add_library(strutil STATIC lib/libstrutil.c)
target_include_directories(strutil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# Universal module: `interface: "universal"` in metadata.json makes the builder
# run `logos-cpp-generator --from-header` in preConfigure, emitting the Qt
# plugin glue, the dispatch and the C-ABI exports into generated_code/ —
# LogosModule.cmake picks those up automatically, so no loader / interface
# header is listed here. `strutil` stays: the external C library is what this
# fixture exists to exercise.
logos_module(
    NAME test_extlib_module
    SOURCES
        src/test_extlib_module_impl.h
        src/test_extlib_module_impl.cpp
    LINK_TARGETS
        strutil
)
