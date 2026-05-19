cmake_minimum_required(VERSION 3.14)
project(TestContextModuleCppPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Universal module: `interface: "universal"` in metadata.json makes the
# builder run `logos-cpp-generator --from-header` in preConfigure. The
# generator produces the Qt glue + dispatch which LogosModule.cmake
# picks up from generated_code/ automatically.
logos_module(
    NAME test_context_module_cpp
    SOURCES
        src/test_context_module_cpp_impl.h
        src/test_context_module_cpp_impl.cpp
)
