cmake_minimum_required(VERSION 3.14)
project(TestExtlibModulePlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

logos_module(
    NAME test_extlib_module
    SOURCES
        src/test_extlib_module_interface.h
        src/test_extlib_module_plugin.h
        src/test_extlib_module_plugin.cpp
    EXTERNAL_LIBS
        strutil
)
