#include "test_qtbind_probe_impl.h"

#include <QDebug>

// Generated at build time. Because metadata.json declares
// `interface_dependencies`, this umbrella carries a
// `FullApi bind_full_api(const QString&)` factory. Because the module has NO
// `interface` key (type: core), the backend picks apiStyle=qt, so `FullApi` is
// the Qt-TYPED wrapper (QString / QByteArray / qlonglong / qulonglong /
// QVariantList / QVariantMap / LogosResult).
#include "logos_sdk.h"

void TestQtbindProbeImpl::onInit(LogosAPI* api)
{
    m_api = api;
    delete m_logos;
    m_logos = new LogosModules(api);
    qDebug() << "TestQtbindProbeImpl: initialized (qt api style, bound interface)";
}

bool TestQtbindProbeImpl::useProvider(const QString& moduleName)
{
    if (!m_logos) return false;
    m_providerName = moduleName;
    delete m_target;
    // The whole point of the probe: a runtime-bound Qt-typed interface wrapper.
    m_target = new FullApi(m_logos->bind_full_api(moduleName));

    // ONE event: subscribe through the generated typed accessor.
    m_target->onIntEvent([this](qlonglong v) {
        m_lastEvent = QString("intEvent:%1").arg(v);
        qDebug() << "TestQtbindProbeImpl: got intEvent" << v;
        emitEvent("intEvent", QVariantList() << QVariant::fromValue(v));
    });
    return true;
}

QString TestQtbindProbeImpl::currentProvider()
{
    return m_providerName;
}

qlonglong TestQtbindProbeImpl::echoInt(qlonglong v)
{
    if (!m_target) return 0;
    return m_target->echoInt(v);
}

bool TestQtbindProbeImpl::fireIntEvent(qlonglong v)
{
    if (!m_target) return false;
    return m_target->fireIntEvent(v);
}

QString TestQtbindProbeImpl::getLastEvent()
{
    return m_lastEvent;
}
