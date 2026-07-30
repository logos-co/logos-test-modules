cmake_minimum_required(VERSION 3.14)
project(TestFullapiQtproxyPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Qt api-style module (type: core, no `interface` key) with an INTERFACE
# dependency: metadata.json declares full_api = interfaces/full_api.lidl, so the
# builder generates the Qt-typed bound wrapper `bind_full_api(name)` into
# generated_code/ (globbed automatically by LogosModule.cmake). The provider
# dispatch (logos_provider_dispatch.cpp) is produced by the flake's preConfigure
# and MUST be listed here — an unlisted impl TU is never compiled and the plugin
# crashes on its first call.
logos_module(
    NAME test_fullapi_qtproxy
    PROVIDER_HEADER src/test_fullapi_qtproxy_impl.h
    SOURCES
        src/test_fullapi_qtproxy_loader.h
        src/test_fullapi_qtproxy_impl.h
        src/test_fullapi_qtproxy_impl.cpp
        # SPIKE — the Qt surface as a veneer over the lp wrapper. full_api_lp.*
        # is verbatim `--api-style lp` generator output (class renamed);
        # full_api_veneer.* is the convert-plus-delegate Qt skin over it.
        src/full_api_lp.h
        src/full_api_lp.cpp
        src/full_api_veneer.h
        src/full_api_veneer.cpp
        logos_provider_dispatch.cpp
)
