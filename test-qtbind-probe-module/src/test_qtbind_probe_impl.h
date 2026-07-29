#ifndef TEST_QTBIND_PROBE_IMPL_H
#define TEST_QTBIND_PROBE_IMPL_H

#include "logos_provider_object.h"
#include "logos_api.h"
#include "logos_api_client.h"
#include "logos_sdk.h"
#include "logos_types.h"

// THROWAWAY PROBE. Binds the `full_api` interface at runtime through the
// generated Qt-typed wrapper (`modules().bind_full_api(name)`), forwards ONE
// method and subscribes to ONE event. Nothing else.
class TestQtbindProbeImpl : public LogosProviderBase
{
    LOGOS_PROVIDER(TestQtbindProbeImpl, "test_qtbind_probe", "1.0.0")

protected:
    void onInit(LogosAPI* api) override;

public:
    // Bind the interface to a concrete provider module name at runtime.
    LOGOS_METHOD bool useProvider(const QString& moduleName);
    LOGOS_METHOD QString currentProvider();

    // The one forwarded method.
    LOGOS_METHOD qlonglong echoInt(qlonglong v);

    // The one event: subscribe to the target's `intEvent`, record what arrived.
    LOGOS_METHOD bool fireIntEvent(qlonglong v);
    LOGOS_METHOD QString getLastEvent();

private:
    LogosAPI* m_api = nullptr;
    LogosModules* m_logos = nullptr;
    FullApi* m_target = nullptr;
    QString m_providerName;
    QString m_lastEvent;
};

#endif // TEST_QTBIND_PROBE_IMPL_H
