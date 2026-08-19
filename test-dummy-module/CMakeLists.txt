cmake_minimum_required(VERSION 3.14)
project(DummyModule000000Plugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Universal module: `interface: "universal"` in metadata.json makes the builder
# run three codegen steps in preConfigure —
#   logos-cpp-generator   --header-to-lidl src/<impl>.h   -> generated_code/<name>.lidl
#   logos-qt-host-generator --lidl <name>.lidl --backend cdylib
#                                                         -> <name>_cdylib_glue.{h,cpp}
#   logos-cpp-generator   --lidl <name>.lidl --backend cdylib
#                            -> <name>_module_impl.cpp, <name>_types.h
# — and LogosModule.cmake picks all of them up from generated_code/, so no loader or
# dispatch source is listed here.
# (The Qt glue comes from logos-plugin-qt's logos-qt-host-generator, not from
# logos-cpp-generator; logos-cpp-generator's own `--backend qt` was removed.)
logos_module(
    NAME dummy_module_000000
    SOURCES
        src/dummy_module_000000_impl.h
        src/dummy_module_000000_impl.cpp
)
