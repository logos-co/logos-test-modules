cmake_minimum_required(VERSION 3.14)
project(TestFullapiProxyPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Universal module with an interface dependency: metadata.json declares
# `interface_dependencies` (full_api = interfaces/full_api.h = IFullApi), so the
# builder generates a bound wrapper `bind_full_api(name)` alongside the usual Qt
# glue. LogosModule.cmake globs generated_code/ automatically.
logos_module(
    NAME test_fullapi_proxy
    SOURCES
        src/test_fullapi_proxy_impl.h
        src/test_fullapi_proxy_impl.cpp
)
