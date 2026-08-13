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

# logos-cpp-sdk (Qt-free base SDK headers; logos_api.h moved to logos-qt-sdk)
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

# logos-qt-sdk (Qt developer layer: LogosAPI, provider objects)
if(NOT DEFINED LOGOS_QT_SDK_ROOT)
    if(DEFINED ENV{LOGOS_QT_SDK_ROOT})
        set(LOGOS_QT_SDK_ROOT "$ENV{LOGOS_QT_SDK_ROOT}")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../logos-qt-sdk/cpp/logos_api.h")
        get_filename_component(LOGOS_QT_SDK_ROOT
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../logos-qt-sdk" ABSOLUTE)
    else()
        message(FATAL_ERROR "logos-qt-sdk not found. Set LOGOS_QT_SDK_ROOT.")
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
if(EXISTS "${LOGOS_QT_SDK_ROOT}/cpp/logos_api.h")
    set(LOGOS_QT_SDK_IS_SOURCE TRUE)
    set(LOGOS_QT_SDK_INCLUDE "${LOGOS_QT_SDK_ROOT}/cpp")
    set(LOGOS_QT_SDK_CORE_INCLUDE "${LOGOS_QT_SDK_ROOT}/core")
else()
    set(LOGOS_QT_SDK_IS_SOURCE FALSE)
    set(LOGOS_QT_SDK_INCLUDE "${LOGOS_QT_SDK_ROOT}/include/cpp")
    set(LOGOS_QT_SDK_CORE_INCLUDE "${LOGOS_QT_SDK_ROOT}/include/core")
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

# Generated code lives next to the module, not in tests/.
set(GENERATED_CODE_DIR_EARLY "${CMAKE_CURRENT_SOURCE_DIR}/../generated_code")

# The impl is a universal module: a plain Qt-free class. There is no
# logos_provider_dispatch.cpp and no generated loader to compile here — the
# tests drive the impl directly and inject the generated LogosModules umbrella.
set(TEST_SOURCES
    main.cpp
    test_call_basic.cpp
    test_call_extlib.cpp
    test_chaining.cpp
    ../src/test_ipc_new_api_impl.cpp
)

# The generated bodies of the `logos_events:` declarations. The impl calls
# triggeredBasicEvent(), so this is a LINK requirement, not optional extra
# coverage — without it the binary fails on an undefined symbol.
set(EVENTS_CDYLIB "${GENERATED_CODE_DIR_EARLY}/test_ipc_new_api_module_events_cdylib.cpp")
if(EXISTS "${EVENTS_CDYLIB}")
    list(APPEND TEST_SOURCES "${EVENTS_CDYLIB}")
    set_source_files_properties("${EVENTS_CDYLIB}" PROPERTIES SKIP_AUTOMOC ON)
else()
    message(FATAL_ERROR
        "Generated event bodies not found at ${EVENTS_CDYLIB}. The Nix buildPhase "
        "emits them with `logos-cpp-generator --lidl ... --backend cdylib`; without "
        "them this target fails to link on TestIpcNewApiImpl::triggeredBasicEvent.")
endif()

if(LOGOS_QT_SDK_IS_SOURCE)
    # Source layout: compile the Qt developer layer into the test binary.
    # The transport/consumer core (types, api_client, token_manager, mock
    # transports, ...) lives in the logos-protocol LIBRARY and is linked
    # below instead of compiled in.
    list(APPEND TEST_SOURCES
        ${LOGOS_QT_SDK_INCLUDE}/logos_api.cpp
        ${LOGOS_QT_SDK_INCLUDE}/logos_api_provider.cpp
        ${LOGOS_QT_SDK_INCLUDE}/logos_provider_object.cpp
        ${LOGOS_QT_SDK_INCLUDE}/qt_provider_object.cpp
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
    ${LOGOS_QT_SDK_INCLUDE}
    ${LOGOS_QT_SDK_CORE_INCLUDE}
    ${LOGOS_PROTOCOL_INCLUDE}
    ${LOGOS_PROTOCOL_IMPL_INCLUDE}/qt_local
    ${LOGOS_PROTOCOL_IMPL_INCLUDE}/qt_remote
    ${LOGOS_CPP_SDK_MOCK_INCLUDE}
    ${GENERATED_CODE_DIR}
    ${GENERATED_CODE_DIR}/include
)

target_link_libraries(test_ipc_new_api_module_tests PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::RemoteObjects
)

# Link the Qt SDK via its exported CMake target so the consumer inherits
# the full transitive link interface (logos-protocol, and through it
# OpenSSL::SSL, OpenSSL::Crypto, Boost::system, nlohmann_json). A bare
# archive on the link line would leave Boost.Asio TLS symbols undefined.
if(NOT LOGOS_QT_SDK_IS_SOURCE)
    find_package(logos-protocol REQUIRED CONFIG
        PATHS ${LOGOS_PROTOCOL_ROOT}/lib/cmake/logos-protocol
        NO_DEFAULT_PATH)
    find_package(logos-qt-sdk REQUIRED CONFIG
        PATHS ${LOGOS_QT_SDK_ROOT}/lib/cmake/logos-qt-sdk
        NO_DEFAULT_PATH)
    target_link_libraries(test_ipc_new_api_module_tests PRIVATE logos-qt-sdk::logos_qt_sdk)
else()
    # Source-layout qt-sdk compiled in above; link the protocol library.
    if(NOT TARGET logos_protocol)
        add_subdirectory("${LOGOS_PROTOCOL_ROOT}/cpp"
                         "${CMAKE_BINARY_DIR}/logos-protocol-build")
    endif()
    target_link_libraries(test_ipc_new_api_module_tests PRIVATE logos_protocol)
endif()

# Qt-free base SDK headers (nlohmann include path for logos_json.h etc.)
if(EXISTS "${LOGOS_CPP_SDK_ROOT}/lib/cmake/logos-cpp-sdk")
    find_package(logos-cpp-sdk REQUIRED CONFIG
        PATHS ${LOGOS_CPP_SDK_ROOT}/lib/cmake/logos-cpp-sdk
        NO_DEFAULT_PATH)
    target_link_libraries(test_ipc_new_api_module_tests PRIVATE logos-cpp-sdk::logos_headers)
else()
    find_package(nlohmann_json REQUIRED)
    target_link_libraries(test_ipc_new_api_module_tests PRIVATE nlohmann_json::nlohmann_json)
endif()

# ── CTest integration ────────────────────────────────────────────────────────

enable_testing()
add_test(NAME logos_new_api_unit_tests COMMAND test_ipc_new_api_module_tests)
