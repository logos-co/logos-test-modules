cmake_minimum_required(VERSION 3.14)
project(TestFullapiQtproxyPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Universal (header-first cdylib) module with an INTERFACE dependency and a
# QT-TYPED consumer surface:
#
#   * `interface: "universal"` — the builder derives the LIDL contract from
#     src/test_fullapi_qtproxy_impl.h, emits the C ABI provider wrapper and the
#     uniform Qt plugin glue into generated_code/. There is no PROVIDER_HEADER
#     and no hand-listed dispatch TU: the `--provider-header` generator mode that
#     produced logos_provider_dispatch.cpp was removed, and this module was its
#     last caller.
#   * `codegen.consumer_api_style: "qt"` — metadata.json declares full_api =
#     interfaces/full_api.lidl, and the wrapper generated for it is the Qt-typed,
#     origin-bound `bind_full_api(name)`.
#
# LogosModule.cmake globs generated_code/ automatically, so only the hand-written
# impl is listed here.
logos_module(
    NAME test_fullapi_qtproxy
    SOURCES
        src/test_fullapi_qtproxy_impl.h
        src/test_fullapi_qtproxy_impl.cpp
)
