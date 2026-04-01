cmake_minimum_required(VERSION 3.14)
project(DummyModule000000Plugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

logos_module(
    NAME dummy_module_000000
    PROVIDER_HEADER src/dummy_module_000000_impl.h
    SOURCES
        src/dummy_module_000000_loader.h
        src/dummy_module_000000_impl.h
        src/dummy_module_000000_impl.cpp
        logos_provider_dispatch.cpp
)
