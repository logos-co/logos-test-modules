#include "test_ipc_new_api_impl.h"
#include <iostream>

void TestIpcNewApiImpl::onInit(NativeLogosAPI* api)
{
    m_nativeApi = api;
    m_basicClient = api->getClient("test_basic_module");
    m_extlibClient = api->getClient("test_extlib_module");
    std::cerr << "TestIpcNewApiImpl: NativeLogosAPI initialized (native provider API)\n";
}

// -- Calls to test_basic_module --

std::string TestIpcNewApiImpl::callBasicEcho(const std::string& input)
{
    LogosValue result = m_basicClient->invokeMethod(
        "test_basic_module", "echo", LogosValue(input));
    return result.toString();
}

int TestIpcNewApiImpl::callBasicAddInts(int a, int b)
{
    LogosValue result = m_basicClient->invokeMethod(
        "test_basic_module", "addInts", LogosValue(a), LogosValue(b));
    return static_cast<int>(result.toInt());
}

bool TestIpcNewApiImpl::callBasicReturnTrue()
{
    LogosValue result = m_basicClient->invokeMethod(
        "test_basic_module", "returnTrue");
    return result.toBool();
}

std::string TestIpcNewApiImpl::callBasicNoArgs()
{
    LogosValue result = m_basicClient->invokeMethod(
        "test_basic_module", "noArgs");
    return result.toString();
}

std::string TestIpcNewApiImpl::callBasicFiveArgs(const std::string& a, int b, bool c, const std::string& d, int e)
{
    LogosValue result = m_basicClient->invokeMethod(
        "test_basic_module", "fiveArgs",
        LogosValue(a), LogosValue(b), LogosValue(c), LogosValue(d), LogosValue(e));
    return result.toString();
}

NativeLogosResult TestIpcNewApiImpl::callBasicSuccessResult()
{
    LogosValue result = m_basicClient->invokeMethod(
        "test_basic_module", "successResult");
    if (result.isMap()) {
        auto m = result.toMap();
        NativeLogosResult nr;
        auto sIt = m.find("success");
        nr.success = (sIt != m.end()) ? sIt->second.toBool() : true;
        auto vIt = m.find("value");
        nr.value = (vIt != m.end()) ? vIt->second : result;
        auto eIt = m.find("error");
        nr.error = (eIt != m.end()) ? eIt->second.toString() : "";
        return nr;
    }
    return {true, result, ""};
}

NativeLogosResult TestIpcNewApiImpl::callBasicErrorResult()
{
    LogosValue result = m_basicClient->invokeMethod(
        "test_basic_module", "errorResult");
    if (result.isMap()) {
        auto m = result.toMap();
        NativeLogosResult nr;
        auto sIt = m.find("success");
        nr.success = (sIt != m.end()) ? sIt->second.toBool() : false;
        auto vIt = m.find("value");
        nr.value = (vIt != m.end()) ? vIt->second : LogosValue();
        auto eIt = m.find("error");
        nr.error = (eIt != m.end()) ? eIt->second.toString() : "";
        return nr;
    }
    return {false, LogosValue(), result.toString()};
}

std::string TestIpcNewApiImpl::callBasicResultMapField(const std::string& key)
{
    LogosValue result = m_basicClient->invokeMethod(
        "test_basic_module", "resultWithMap");
    if (result.isMap()) {
        auto m = result.toMap();
        auto sIt = m.find("success");
        bool success = (sIt != m.end()) ? sIt->second.toBool() : false;
        if (success) {
            auto vIt = m.find("value");
            if (vIt != m.end() && vIt->second.isMap()) {
                auto innerMap = vIt->second.toMap();
                auto kIt = innerMap.find(key);
                if (kIt != innerMap.end())
                    return kIt->second.toString();
            }
        }
    }
    return "";
}

// -- Calls to test_extlib_module --

std::string TestIpcNewApiImpl::callExtlibReverse(const std::string& input)
{
    LogosValue result = m_extlibClient->invokeMethod(
        "test_extlib_module", "reverseString", LogosValue(input));
    return result.toString();
}

std::string TestIpcNewApiImpl::callExtlibUppercase(const std::string& input)
{
    LogosValue result = m_extlibClient->invokeMethod(
        "test_extlib_module", "uppercaseString", LogosValue(input));
    return result.toString();
}

int TestIpcNewApiImpl::callExtlibCountChars(const std::string& input)
{
    LogosValue result = m_extlibClient->invokeMethod(
        "test_extlib_module", "countChars", LogosValue(input));
    return static_cast<int>(result.toInt());
}

// -- Cross-module chaining --

std::string TestIpcNewApiImpl::chainEchoThenReverse(const std::string& input)
{
    LogosValue echoed = m_basicClient->invokeMethod(
        "test_basic_module", "echo", LogosValue(input));
    LogosValue reversed = m_extlibClient->invokeMethod(
        "test_extlib_module", "reverseString", echoed);
    return reversed.toString();
}

std::string TestIpcNewApiImpl::chainUppercaseThenConcat(const std::string& a, const std::string& b)
{
    LogosValue upperA = m_extlibClient->invokeMethod(
        "test_extlib_module", "uppercaseString", LogosValue(a));
    LogosValue upperB = m_extlibClient->invokeMethod(
        "test_extlib_module", "uppercaseString", LogosValue(b));
    LogosValue result = m_basicClient->invokeMethod(
        "test_basic_module", "concat", upperA, upperB);
    return result.toString();
}

// -- Generated type-safe wrappers (would use native LogosModules) --

std::string TestIpcNewApiImpl::wrapperBasicEcho(const std::string& input)
{
    if (!m_basicClient) return "";
    LogosValue result = m_basicClient->invokeMethod(
        "test_basic_module", "echo", LogosValue(input));
    return result.toString();
}

std::string TestIpcNewApiImpl::wrapperExtlibReverse(const std::string& input)
{
    if (!m_extlibClient) return "";
    LogosValue result = m_extlibClient->invokeMethod(
        "test_extlib_module", "reverseString", LogosValue(input));
    return result.toString();
}

// -- Events --

void TestIpcNewApiImpl::triggerBasicEvent(const std::string& data)
{
    m_basicClient->invokeMethod(
        "test_basic_module", "emitTestEvent", LogosValue(data));
    emitEvent("triggeredBasicEvent", {LogosValue(data)});
}
