#ifndef TEST_IPC_NEW_API_IMPL_H
#define TEST_IPC_NEW_API_IMPL_H

#include "logos_native_provider.h"
#include "logos_native_api.h"
#include "logos_native_client.h"
#include "logos_native_types.h"

class TestIpcNewApiImpl : public NativeProviderBase
{
    NATIVE_LOGOS_PROVIDER(TestIpcNewApiImpl, "test_ipc_new_api_module", "1.0.0")

protected:
    void onInit(NativeLogosAPI* api) override;

public:
    LOGOS_METHOD std::string callBasicEcho(const std::string& input);
    LOGOS_METHOD int callBasicAddInts(int a, int b);
    LOGOS_METHOD bool callBasicReturnTrue();
    LOGOS_METHOD std::string callBasicNoArgs();
    LOGOS_METHOD std::string callBasicFiveArgs(const std::string& a, int b, bool c, const std::string& d, int e);
    LOGOS_METHOD NativeLogosResult callBasicSuccessResult();
    LOGOS_METHOD NativeLogosResult callBasicErrorResult();
    LOGOS_METHOD std::string callBasicResultMapField(const std::string& key);
    LOGOS_METHOD std::string callExtlibReverse(const std::string& input);
    LOGOS_METHOD std::string callExtlibUppercase(const std::string& input);
    LOGOS_METHOD int callExtlibCountChars(const std::string& input);
    LOGOS_METHOD std::string chainEchoThenReverse(const std::string& input);
    LOGOS_METHOD std::string chainUppercaseThenConcat(const std::string& a, const std::string& b);
    LOGOS_METHOD std::string wrapperBasicEcho(const std::string& input);
    LOGOS_METHOD std::string wrapperExtlibReverse(const std::string& input);
    LOGOS_METHOD void triggerBasicEvent(const std::string& data);

private:
    NativeLogosAPI* m_nativeApi = nullptr;
    NativeLogosClient* m_basicClient = nullptr;
    NativeLogosClient* m_extlibClient = nullptr;
};

#endif // TEST_IPC_NEW_API_IMPL_H
