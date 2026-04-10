#pragma once
#include <QString>
#include <QVariantList>
#include "test_qml_backend_interface.h"
#include "LogosViewPluginBase.h"
#include "rep_test_qml_backend_source.h"

class LogosAPI;
class LogosModules;

class TestQmlBackendPlugin : public TestQmlBackendSimpleSource,
                             public TestQmlBackendInterface,
                             public TestQmlBackendViewPluginBase
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID TestQmlBackendInterface_iid FILE "metadata.json")
    Q_INTERFACES(TestQmlBackendInterface)

public:
    explicit TestQmlBackendPlugin(QObject* parent = nullptr);
    ~TestQmlBackendPlugin() override;

    QString name()    const override { return "test_qml_backend"; }
    QString version() const override { return "1.0.0"; }

    Q_INVOKABLE void initLogos(LogosAPI* api);

    // Slots from .rep — call test_basic_module via LogosModules SDK
    int add(int a, int b) override;
    QString echo(QString msg) override;

signals:
    void eventResponse(const QString& eventName, const QVariantList& args);

private:
    LogosAPI* m_logosAPI = nullptr;
    LogosModules* m_logos = nullptr;
};
