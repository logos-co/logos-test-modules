#include "test_uiqml_probe_backend.h"

#include <QJsonDocument>
#include <QJsonValue>
#include <QVariant>

// Generated umbrella: LogosModules (behind modules()) carrying the Qt-typed
// test_fullapi_cpp wrapper. `type: ui_qml` keeps api-style=qt, so these
// signatures are qulonglong / qlonglong / QStringList / QVariantList /
// QVariantMap — exactly the types the .rep slots declare.
#include "logos_sdk.h"

namespace {
// Render any answer as compact JSON so the probe's single lastResult PROP can
// carry every case's shape verbatim.
QString render(const QVariant& v)
{
    return QString::fromUtf8(
        QJsonDocument::fromVariant(QVariantList{v}).toJson(QJsonDocument::Compact));
}
} // namespace

void TestUiqmlProbeBackend::onContextReady()
{
    setLastResult(QStringLiteral("READY"));
}

void TestUiqmlProbeBackend::doEchoUint(qulonglong v)
{
    setLastResult(render(QVariant::fromValue(modules().test_fullapi_cpp.echoUint(v))));
}

void TestUiqmlProbeBackend::doEchoInt(qlonglong v)
{
    setLastResult(render(QVariant::fromValue(modules().test_fullapi_cpp.echoInt(v))));
}

void TestUiqmlProbeBackend::doEchoBool(bool v)
{
    setLastResult(render(QVariant::fromValue(modules().test_fullapi_cpp.echoBool(v))));
}

void TestUiqmlProbeBackend::doEchoStringList(QStringList v)
{
    setLastResult(render(QVariant::fromValue(modules().test_fullapi_cpp.echoStringList(v))));
}

void TestUiqmlProbeBackend::doEchoUintList(QVariantList v)
{
    setLastResult(render(QVariant::fromValue(modules().test_fullapi_cpp.echoUintList(v))));
}

void TestUiqmlProbeBackend::doEchoIntList(QVariantList v)
{
    setLastResult(render(QVariant::fromValue(modules().test_fullapi_cpp.echoIntList(v))));
}

void TestUiqmlProbeBackend::doEchoList(QVariantList v)
{
    setLastResult(render(QVariant::fromValue(modules().test_fullapi_cpp.echoList(v))));
}

void TestUiqmlProbeBackend::doEchoMap(QVariantMap v)
{
    setLastResult(render(QVariant::fromValue(modules().test_fullapi_cpp.echoMap(v))));
}
