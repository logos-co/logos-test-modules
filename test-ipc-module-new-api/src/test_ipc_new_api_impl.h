#ifndef TEST_IPC_NEW_API_IMPL_H
#define TEST_IPC_NEW_API_IMPL_H

#include "logos_provider_object.h"
#include "logos_api.h"
#include "logos_api_client.h"
#include "logos_sdk.h"
#include "logos_types.h"

class TestIpcNewApiImpl : public LogosProviderBase
{
    LOGOS_PROVIDER(TestIpcNewApiImpl, "test_ipc_new_api_module", "1.0.0")

protected:
    void onInit(LogosAPI* api) override;

public:
    LOGOS_METHOD QString callBasicEcho(const QString& input);
    LOGOS_METHOD int callBasicAddInts(int a, int b);
    LOGOS_METHOD bool callBasicReturnTrue();
    LOGOS_METHOD QString callBasicNoArgs();
    LOGOS_METHOD QString callBasicFiveArgs(const QString& a, int b, bool c, const QString& d, int e);
    LOGOS_METHOD LogosResult callBasicSuccessResult();
    LOGOS_METHOD LogosResult callBasicErrorResult();
    LOGOS_METHOD QString callBasicResultMapField(const QString& key);
    LOGOS_METHOD QString callExtlibReverse(const QString& input);
    LOGOS_METHOD QString callExtlibUppercase(const QString& input);
    LOGOS_METHOD int callExtlibCountChars(const QString& input);
    LOGOS_METHOD QString chainEchoThenReverse(const QString& input);
    LOGOS_METHOD QString chainUppercaseThenConcat(const QString& a, const QString& b);
    LOGOS_METHOD QString wrapperBasicEcho(const QString& input);
    LOGOS_METHOD QString wrapperExtlibReverse(const QString& input);
    LOGOS_METHOD void triggerBasicEvent(const QString& data);

private:
    LogosModules* m_logos = nullptr;
    LogosAPIClient* m_basicClient = nullptr;
    LogosAPIClient* m_extlibClient = nullptr;
};

#endif // TEST_IPC_NEW_API_IMPL_H
