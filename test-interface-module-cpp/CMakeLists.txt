cmake_minimum_required(VERSION 3.14)
project(TestInterfaceModuleCppPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Universal module. The builder runs `logos-cpp-generator --from-header`
# to produce the Qt glue, and (because metadata.json declares
# `interface_dependencies`) `--interface basic_calc=interfaces/basic_calc.h=IBasicCalc`
# to produce the bound `BasicCalc` wrapper + `bind_basic_calc` factory.
# LogosModule.cmake picks up the generated files from generated_code/.
logos_module(
    NAME test_interface_module_cpp
    SOURCES
        src/test_interface_module_cpp_impl.h
        src/test_interface_module_cpp_impl.cpp
)
