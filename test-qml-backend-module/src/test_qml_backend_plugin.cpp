#include "test_qml_backend_plugin.h"
#include "logos_api.h"
#include "logos_sdk.h"
#include <QDebug>

TestQmlBackendPlugin::TestQmlBackendPlugin(QObject* parent)
    : TestQmlBackendSimpleSource(parent) {}

TestQmlBackendPlugin::~TestQmlBackendPlugin() { delete m_logos; }

void TestQmlBackendPlugin::initLogos(LogosAPI* api)
{
    if (m_logos) return;
    m_logosAPI = api;
    m_logos = new LogosModules(api);
    setBackend(this);
    setStatus("Ready");
    qDebug() << "TestQmlBackendPlugin: initialized";
}

int TestQmlBackendPlugin::add(int a, int b)
{
    // Call test_basic_module.add() via the typed SDK — proves backend → core IPC
    int result = m_logos->test_basic_module.add(a, b);
    setStatus(QStringLiteral("%1 + %2 = %3 (via test_basic_module)").arg(a).arg(b).arg(result));
    return result;
}

QString TestQmlBackendPlugin::echo(QString msg)
{
    setStatus(QStringLiteral("echo: %1").arg(msg));
    return msg;
}
