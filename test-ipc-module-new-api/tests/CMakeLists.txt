cmake_minimum_required(VERSION 3.14)
project(TestIpcNewApiModuleTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ── Locate dependencies ──────────────────────────────────────────────────────

# logos-liblogos (for PluginInterface / interface.h)
if(NOT DEFINED LOGOS_LIBLOGOS_ROOT)
    if(DEFINED ENV{LOGOS_LIBLOGOS_ROOT})
        set(LOGOS_LIBLOGOS_ROOT "$ENV{LOGOS_LIBLOGOS_ROOT}")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../logos-liblogos/interface.h")
        get_filename_component(LOGOS_LIBLOGOS_ROOT
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../logos-liblogos" ABSOLUTE)
    else()
        message(FATAL_ERROR "logos-liblogos not found. Set LOGOS_LIBLOGOS_ROOT.")
    endif()
endif()

# logos-cpp-sdk (SDK headers + implementation sources)
if(NOT DEFINED LOGOS_CPP_SDK_ROOT)
    if(DEFINED ENV{LOGOS_CPP_SDK_ROOT})
        set(LOGOS_CPP_SDK_ROOT "$ENV{LOGOS_CPP_SDK_ROOT}")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../logos-cpp-sdk/cpp/logos_api.h")
        get_filename_component(LOGOS_CPP_SDK_ROOT
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../logos-cpp-sdk" ABSOLUTE)
    else()
        message(FATAL_ERROR "logos-cpp-sdk not found. Set LOGOS_CPP_SDK_ROOT.")
    endif()
endif()

# Detect whether we have a source layout or an installed layout.
if(EXISTS "${LOGOS_CPP_SDK_ROOT}/cpp/logos_api.h")
    set(LOGOS_CPP_SDK_IS_SOURCE TRUE)
    set(LOGOS_CPP_SDK_INCLUDE "${LOGOS_CPP_SDK_ROOT}/cpp")
    set(LOGOS_CPP_SDK_MOCK_INCLUDE "${LOGOS_CPP_SDK_ROOT}/cpp/implementations/mock")
else()
    set(LOGOS_CPP_SDK_IS_SOURCE FALSE)
    set(LOGOS_CPP_SDK_INCLUDE "${LOGOS_CPP_SDK_ROOT}/include/cpp")
    set(LOGOS_CPP_SDK_MOCK_INCLUDE "${LOGOS_CPP_SDK_ROOT}/include/cpp/implementations/mock")
endif()

if(EXISTS "${LOGOS_LIBLOGOS_ROOT}/interface.h")
    set(LOGOS_LIBLOGOS_INCLUDE "${LOGOS_LIBLOGOS_ROOT}")
elseif(EXISTS "${LOGOS_LIBLOGOS_ROOT}/include/core/interface.h")
    set(LOGOS_LIBLOGOS_INCLUDE "${LOGOS_LIBLOGOS_ROOT}/include/core")
else()
    set(LOGOS_LIBLOGOS_INCLUDE "${LOGOS_LIBLOGOS_ROOT}/include")
endif()

message(STATUS "logos-liblogos include : ${LOGOS_LIBLOGOS_INCLUDE}")
message(STATUS "logos-cpp-sdk include  : ${LOGOS_CPP_SDK_INCLUDE}")
message(STATUS "logos-cpp-sdk is source: ${LOGOS_CPP_SDK_IS_SOURCE}")

# ── Qt ───────────────────────────────────────────────────────────────────────

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core RemoteObjects)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core RemoteObjects)

# ── Provider dispatch code generation ────────────────────────────────────────

# The generated logos_provider_dispatch.cpp must be available at configure time
# or pre-generated.  When building via Nix the preConfigure hook generates it;
# for local dev, run logos-cpp-generator --provider-header manually.
set(PROVIDER_DISPATCH "${CMAKE_CURRENT_SOURCE_DIR}/../logos_provider_dispatch.cpp")
if(NOT EXISTS "${PROVIDER_DISPATCH}")
    message(WARNING "logos_provider_dispatch.cpp not found at ${PROVIDER_DISPATCH} — "
                    "run logos-cpp-generator --provider-header src/test_ipc_new_api_impl.h")
endif()

# ── Source lists ─────────────────────────────────────────────────────────────

set(TEST_SOURCES
    main.cpp
    test_call_basic.cpp
    test_call_extlib.cpp
    test_chaining.cpp
    ../src/test_ipc_new_api_impl.cpp
    ${PROVIDER_DISPATCH}
)

# Mark generated dispatch file — skip AUTOMOC (no Q_OBJECT macros)
set_source_files_properties(${PROVIDER_DISPATCH} PROPERTIES SKIP_AUTOMOC ON)

if(LOGOS_CPP_SDK_IS_SOURCE)
    list(APPEND TEST_SOURCES
        ${LOGOS_CPP_SDK_INCLUDE}/logos_types.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/logos_api.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/logos_api_client.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/logos_api_consumer.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/logos_api_provider.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/module_proxy.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/token_manager.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/logos_transport_factory.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/logos_registry_factory.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/logos_provider_object.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/qt_provider_object.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/implementations/qt_local/local_transport.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/implementations/qt_remote/remote_transport.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/implementations/qt_remote/qt_remote_registry.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/implementations/mock/mock_store.cpp
        ${LOGOS_CPP_SDK_INCLUDE}/implementations/mock/mock_transport.cpp
    )
endif()

# logos_sdk.cpp is the generated wrapper; look in generated_code/ relative to the module.
# SKIP_AUTOMOC is required to avoid a ninja dependency cycle.
set(GENERATED_CODE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../generated_code")
if(EXISTS "${GENERATED_CODE_DIR}/logos_sdk.cpp")
    list(APPEND TEST_SOURCES "${GENERATED_CODE_DIR}/logos_sdk.cpp")
    set_source_files_properties("${GENERATED_CODE_DIR}/logos_sdk.cpp"
        PROPERTIES SKIP_AUTOMOC ON)
elseif(EXISTS "${GENERATED_CODE_DIR}/include/logos_sdk.cpp")
    list(APPEND TEST_SOURCES "${GENERATED_CODE_DIR}/include/logos_sdk.cpp")
    set_source_files_properties("${GENERATED_CODE_DIR}/include/logos_sdk.cpp"
        PROPERTIES SKIP_AUTOMOC ON)
endif()

# ── Test executable ──────────────────────────────────────────────────────────

add_executable(test_ipc_new_api_module_tests ${TEST_SOURCES})

target_include_directories(test_ipc_new_api_module_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LOGOS_LIBLOGOS_INCLUDE}
    ${LOGOS_CPP_SDK_INCLUDE}
    ${LOGOS_CPP_SDK_INCLUDE}/implementations/qt_local
    ${LOGOS_CPP_SDK_INCLUDE}/implementations/qt_remote
    ${LOGOS_CPP_SDK_MOCK_INCLUDE}
    ${GENERATED_CODE_DIR}
    ${GENERATED_CODE_DIR}/include
)

target_link_libraries(test_ipc_new_api_module_tests PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::RemoteObjects
)

# In installed layout, link the SDK via its exported CMake target so
# the consumer inherits INTERFACE_LINK_LIBRARIES (OpenSSL::SSL,
# OpenSSL::Crypto, Boost::system, nlohmann_json). Bare find_library
# only puts liblogos_sdk.a on the link line — Boost.Asio TLS symbols
# (X509_check_host, SSL_*, ...) end up undefined on Linux.
if(NOT LOGOS_CPP_SDK_IS_SOURCE)
    find_package(logos-cpp-sdk REQUIRED CONFIG
        PATHS ${LOGOS_CPP_SDK_ROOT}/lib/cmake/logos-cpp-sdk
        NO_DEFAULT_PATH)
    target_link_libraries(test_ipc_new_api_module_tests PRIVATE logos-cpp-sdk::logos_sdk)
endif()

# ── CTest integration ────────────────────────────────────────────────────────

enable_testing()
add_test(NAME logos_new_api_unit_tests COMMAND test_ipc_new_api_module_tests)
