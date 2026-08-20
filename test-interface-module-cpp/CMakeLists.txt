cmake_minimum_required(VERSION 3.14)
project(TestInterfaceModuleCppPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Universal module. The builder runs `logos-cpp-generator --header-to-lidl`
# on the impl header, then `logos-qt-host-generator --lidl ... --backend cdylib`
# for the Qt plugin glue and `logos-cpp-generator --lidl ... --backend cdylib`
# for the Qt-free C-ABI exports. Because metadata.json declares
# `interface_dependencies` it also runs
# `--interface basic_calc=interfaces/basic_calc.h=IBasicCalc`
# to produce the bound `BasicCalc` wrapper + `bind_basic_calc` factory.
# LogosModule.cmake picks up the generated files from generated_code/.
logos_module(
    NAME test_interface_module_cpp
    SOURCES
        src/test_interface_module_cpp_impl.h
        src/test_interface_module_cpp_impl.cpp
)
