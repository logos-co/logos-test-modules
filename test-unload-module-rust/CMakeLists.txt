cmake_minimum_required(VERSION 3.14)
project(TestUnloadModuleRustPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LOGOS_MODULE_BUILDER_ROOT is not set.")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/metadata.json
               ${CMAKE_CURRENT_BINARY_DIR}/metadata.json COPYONLY)

# Rust-first: the builder derives the .lidl from the TestUnloadModuleRust trait
# in src/lib.rs, generates the scaffold from it, and links the crate archive.
logos_module(NAME test_unload_module_rust)
