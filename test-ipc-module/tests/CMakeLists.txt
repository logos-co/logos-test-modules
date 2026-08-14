cmake_minimum_required(VERSION 3.14)
project(TestIpcModuleTests LANGUAGES CXX)

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

# logos-cpp-sdk (Qt-free base SDK headers; logos_api.h lives in logos-qt-host)
if(NOT DEFINED LOGOS_CPP_SDK_ROOT)
    if(DEFINED ENV{LOGOS_CPP_SDK_ROOT})
        set(LOGOS_CPP_SDK_ROOT "$ENV{LOGOS_CPP_SDK_ROOT}")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../logos-cpp-sdk/cpp/logos_module_context.h")
        get_filename_component(LOGOS_CPP_SDK_ROOT
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../logos-cpp-sdk" ABSOLUTE)
    else()
        message(FATAL_ERROR "logos-cpp-sdk not found. Set LOGOS_CPP_SDK_ROOT.")
    endif()
endif()

# logos-qt-host (Qt HOST RUNTIME: LogosAPI, LogosAPIProvider, the provider base
# classes and the legacy QMetaObject adapter). It is built from logos-plugin-qt
# and exported as the logos-qt-host::logos_qt_host target; logos-qt-sdk no
# longer carries it, so the source-checkout fallback names the REPO
# (logos-plugin-qt) while the installed prefix is the logos-qt-host package.
if(NOT DEFINED LOGOS_QT_HOST_ROOT)
    if(DEFINED ENV{LOGOS_QT_HOST_ROOT})
        set(LOGOS_QT_HOST_ROOT "$ENV{LOGOS_QT_HOST_ROOT}")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../logos-plugin-qt/cpp/logos_api.h")
        get_filename_component(LOGOS_QT_HOST_ROOT
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../logos-plugin-qt" ABSOLUTE)
    else()
        message(FATAL_ERROR
            "logos-qt-host not found. Set LOGOS_QT_HOST_ROOT to an installed "
            "logos-qt-host prefix or to a logos-plugin-qt source checkout.")
    endif()
endif()

# logos-protocol (transports, consumer core, mock implementations)
if(NOT DEFINED LOGOS_PROTOCOL_ROOT)
    if(DEFINED ENV{LOGOS_PROTOCOL_ROOT})
        set(LOGOS_PROTOCOL_ROOT "$ENV{LOGOS_PROTOCOL_ROOT}")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../logos-protocol/cpp/logos_protocol.h")
        get_filename_component(LOGOS_PROTOCOL_ROOT
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../logos-protocol" ABSOLUTE)
    else()
        message(FATAL_ERROR "logos-protocol not found. Set LOGOS_PROTOCOL_ROOT.")
    endif()
endif()

# Detect source vs installed layouts.
if(EXISTS "${LOGOS_CPP_SDK_ROOT}/cpp/logos_module_context.h")
    set(LOGOS_CPP_SDK_IS_SOURCE TRUE)
    set(LOGOS_CPP_SDK_INCLUDE "${LOGOS_CPP_SDK_ROOT}/cpp")
else()
    set(LOGOS_CPP_SDK_IS_SOURCE FALSE)
    set(LOGOS_CPP_SDK_INCLUDE "${LOGOS_CPP_SDK_ROOT}/include/cpp")
endif()
if(EXISTS "${LOGOS_QT_HOST_ROOT}/cpp/logos_api.h")
    set(LOGOS_QT_HOST_IS_SOURCE TRUE)
    set(LOGOS_QT_HOST_INCLUDE "${LOGOS_QT_HOST_ROOT}/cpp")
    set(LOGOS_QT_HOST_CORE_INCLUDE "${LOGOS_QT_HOST_ROOT}/core")
else()
    set(LOGOS_QT_HOST_IS_SOURCE FALSE)
    set(LOGOS_QT_HOST_INCLUDE "${LOGOS_QT_HOST_ROOT}/include/cpp")
    set(LOGOS_QT_HOST_CORE_INCLUDE "${LOGOS_QT_HOST_ROOT}/include/core")
endif()
if(EXISTS "${LOGOS_PROTOCOL_ROOT}/cpp/logos_protocol.h")
    set(LOGOS_PROTOCOL_INCLUDE "${LOGOS_PROTOCOL_ROOT}/cpp")
    set(LOGOS_PROTOCOL_IMPL_INCLUDE "${LOGOS_PROTOCOL_ROOT}/cpp/implementations")
else()
    set(LOGOS_PROTOCOL_INCLUDE "${LOGOS_PROTOCOL_ROOT}/include")
    set(LOGOS_PROTOCOL_IMPL_INCLUDE "${LOGOS_PROTOCOL_ROOT}/include/implementations")
endif()
# Mock transport headers live in logos-protocol since the extraction.
set(LOGOS_CPP_SDK_MOCK_INCLUDE "${LOGOS_PROTOCOL_IMPL_INCLUDE}/mock")

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

# ── Source lists ─────────────────────────────────────────────────────────────

set(TEST_SOURCES
    main.cpp
    test_call_basic.cpp
    test_call_extlib.cpp
    test_chaining.cpp
    ../src/test_ipc_module_plugin.cpp
)

if(LOGOS_QT_HOST_IS_SOURCE)
    # Source layout: compile the Qt host runtime into the test binary.
    # The transport/consumer core (types, api_client, token_manager, mock
    # transports, ...) lives in the logos-protocol LIBRARY and is linked
    # below instead of compiled in.
    list(APPEND TEST_SOURCES
        ${LOGOS_QT_HOST_INCLUDE}/logos_api.cpp
        ${LOGOS_QT_HOST_INCLUDE}/logos_api_provider.cpp
        ${LOGOS_QT_HOST_INCLUDE}/logos_provider_object.cpp
        ${LOGOS_QT_HOST_INCLUDE}/qt_provider_object.cpp
        # qt_provider_object.cpp decodes every incoming argument through
        # logos::qtArgDecode, so the legacy adapter belongs to the source
        # layout too. Listed unconditionally: a checkout that lacks it must
        # fail at configure time, not silently drop the translation unit and
        # then fail on an undefined symbol at link.
        ${LOGOS_QT_HOST_INCLUDE}/logos_qt_arg_decode.cpp
    )
endif()

# logos_sdk.cpp is the generated wrapper; look in generated_code/ relative to the module.
# SKIP_AUTOMOC is required to avoid a ninja dependency cycle — this file has no Q_OBJECT macros.
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

add_executable(test_ipc_module_tests ${TEST_SOURCES})

target_include_directories(test_ipc_module_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LOGOS_LIBLOGOS_INCLUDE}
    ${LOGOS_CPP_SDK_INCLUDE}
    ${LOGOS_QT_HOST_INCLUDE}
    ${LOGOS_QT_HOST_CORE_INCLUDE}
    ${LOGOS_PROTOCOL_INCLUDE}
    ${LOGOS_PROTOCOL_IMPL_INCLUDE}/qt_local
    ${LOGOS_PROTOCOL_IMPL_INCLUDE}/qt_remote
    ${LOGOS_CPP_SDK_MOCK_INCLUDE}
    ${GENERATED_CODE_DIR}
    ${GENERATED_CODE_DIR}/include
)

target_link_libraries(test_ipc_module_tests PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::RemoteObjects
)

# Link the Qt host runtime via its exported CMake target so the consumer
# inherits the full transitive link interface (logos-protocol, and through it
# OpenSSL::SSL, OpenSSL::Crypto, Boost::system, nlohmann_json). A bare
# archive on the link line would leave Boost.Asio TLS symbols undefined.
#
# find_package(logos-qt-host) is REQUIRED and the target reference below is
# unconditional on purpose: no `if(TARGET ...)` guard, so a renamed or missing
# prefix is a hard configure error rather than a silently unlinked runtime.
if(NOT LOGOS_QT_HOST_IS_SOURCE)
    find_package(logos-protocol REQUIRED CONFIG
        PATHS ${LOGOS_PROTOCOL_ROOT}/lib/cmake/logos-protocol
        NO_DEFAULT_PATH)
    find_package(logos-qt-host REQUIRED CONFIG
        PATHS ${LOGOS_QT_HOST_ROOT}/lib/cmake/logos-qt-host
        NO_DEFAULT_PATH)
    target_link_libraries(test_ipc_module_tests PRIVATE logos-qt-host::logos_qt_host)
else()
    # Source-layout host runtime compiled in above; link the protocol library.
    if(NOT TARGET logos_protocol)
        add_subdirectory("${LOGOS_PROTOCOL_ROOT}/cpp"
                         "${CMAKE_BINARY_DIR}/logos-protocol-build")
    endif()
    target_link_libraries(test_ipc_module_tests PRIVATE logos_protocol)
endif()

# Qt-free base SDK headers (nlohmann include path for logos_json.h etc.)
if(EXISTS "${LOGOS_CPP_SDK_ROOT}/lib/cmake/logos-cpp-sdk")
    find_package(logos-cpp-sdk REQUIRED CONFIG
        PATHS ${LOGOS_CPP_SDK_ROOT}/lib/cmake/logos-cpp-sdk
        NO_DEFAULT_PATH)
    target_link_libraries(test_ipc_module_tests PRIVATE logos-cpp-sdk::logos_headers)
else()
    find_package(nlohmann_json REQUIRED)
    target_link_libraries(test_ipc_module_tests PRIVATE nlohmann_json::nlohmann_json)
endif()

# ── CTest integration ────────────────────────────────────────────────────────

enable_testing()
add_test(NAME logos_module_unit_tests COMMAND test_ipc_module_tests)
