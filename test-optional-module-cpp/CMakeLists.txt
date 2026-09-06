cmake_minimum_required(VERSION 3.14)
project(TestOptionalModuleCppPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Universal module. `optional_dependencies` reaches the codegen through the
# same path `dependencies` does — the builder resolves the dep's published
# LIDL and emits a name-baked modules().test_basic_module_cpp wrapper — so
# there is nothing extra to declare here.
logos_module(
    NAME test_optional_module_cpp
    SOURCES
        src/test_optional_module_cpp_impl.h
        src/test_optional_module_cpp_impl.cpp
)
