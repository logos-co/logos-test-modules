cmake_minimum_required(VERSION 3.14)
project(TestQmlBackendPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found. Set LOGOS_MODULE_BUILDER_ROOT.")
endif()

logos_module(
    NAME test_qml_backend
    REP_FILE src/test_qml_backend.rep
    SOURCES
        src/test_qml_backend_interface.h
        src/test_qml_backend_plugin.h
        src/test_qml_backend_plugin.cpp
)
