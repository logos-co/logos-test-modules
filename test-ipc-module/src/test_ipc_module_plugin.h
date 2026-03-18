#ifndef TEST_IPC_MODULE_PLUGIN_H
#define TEST_IPC_MODULE_PLUGIN_H

#include <QObject>
#include <QString>
#include "test_ipc_module_interface.h"
#include "logos_api.h"
#include "logos_sdk.h"

class TestIpcModulePlugin : public QObject, public TestIpcModuleInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID TestIpcModuleInterface_iid FILE "metadata.json")
    Q_INTERFACES(TestIpcModuleInterface PluginInterface)

public:
    explicit TestIpcModulePlugin(QObject* parent = nullptr);
    ~TestIpcModulePlugin() override;

    QString name() const override { return "test_ipc_module"; }
    QString version() const override { return "1.0.0"; }

    Q_INVOKABLE void initLogos(LogosAPI* logosAPIInstance);

    // ── Calls to test_basic_module ───────────────────────────────────────────
    Q_INVOKABLE QString callBasicEcho(const QString& input) override;
    Q_INVOKABLE int callBasicAddInts(int a, int b) override;
    Q_INVOKABLE bool callBasicReturnTrue() override;
    Q_INVOKABLE QString callBasicNoArgs() override;
    Q_INVOKABLE QString callBasicFiveArgs(const QString& a, int b, bool c, const QString& d, int e) override;
    Q_INVOKABLE LogosResult callBasicSuccessResult() override;
    Q_INVOKABLE LogosResult callBasicErrorResult() override;
    Q_INVOKABLE QString callBasicResultMapField(const QString& key) override;

    // ── Calls to test_extlib_module ──────────────────────────────────────────
    Q_INVOKABLE QString callExtlibReverse(const QString& input) override;
    Q_INVOKABLE QString callExtlibUppercase(const QString& input) override;
    Q_INVOKABLE int callExtlibCountChars(const QString& input) override;

    // ── Cross-module chaining ────────────────────────────────────────────────
    Q_INVOKABLE QString chainEchoThenReverse(const QString& input) override;
    Q_INVOKABLE QString chainUppercaseThenConcat(const QString& a, const QString& b) override;

    // ── Generated wrappers (LogosModules) ────────────────────────────────────
    Q_INVOKABLE QString wrapperBasicEcho(const QString& input) override;
    Q_INVOKABLE QString wrapperExtlibReverse(const QString& input) override;

    // ── Events ───────────────────────────────────────────────────────────────
    Q_INVOKABLE void triggerBasicEvent(const QString& data) override;

    // ── Async calls ───────────────────────────────────────────────────────────
    Q_INVOKABLE QString asyncCallBasicEcho(const QString& input) override;
    Q_INVOKABLE int asyncCallBasicAddInts(int a, int b) override;
    Q_INVOKABLE QString asyncCallExtlibReverse(const QString& input) override;
    Q_INVOKABLE QString asyncWrapperBasicEcho(const QString& input) override;

signals:
    void eventResponse(const QString& eventName, const QVariantList& args);

private:
    LogosModules* logos = nullptr;
    LogosAPIClient* basicClient = nullptr;
    LogosAPIClient* extlibClient = nullptr;
};

#endif // TEST_IPC_MODULE_PLUGIN_H
