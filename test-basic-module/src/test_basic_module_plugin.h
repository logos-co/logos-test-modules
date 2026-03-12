#ifndef TEST_BASIC_MODULE_PLUGIN_H
#define TEST_BASIC_MODULE_PLUGIN_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QUrl>
#include <QVariant>
#include <QJsonArray>
#include "test_basic_module_interface.h"
#include "logos_api.h"
#include "logos_sdk.h"

class TestBasicModulePlugin : public QObject, public TestBasicModuleInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID TestBasicModuleInterface_iid FILE "metadata.json")
    Q_INTERFACES(TestBasicModuleInterface PluginInterface)

public:
    explicit TestBasicModulePlugin(QObject* parent = nullptr);
    ~TestBasicModulePlugin() override;

    QString name() const override { return "test_basic_module"; }
    QString version() const override { return "1.0.0"; }

    Q_INVOKABLE void initLogos(LogosAPI* logosAPIInstance);

    // ── Return type: void ────────────────────────────────────────────────────
    Q_INVOKABLE void doNothing() override;
    Q_INVOKABLE void doNothingWithArgs(const QString& a, int b) override;

    // ── Return type: bool ────────────────────────────────────────────────────
    Q_INVOKABLE bool returnTrue() override;
    Q_INVOKABLE bool returnFalse() override;
    Q_INVOKABLE bool isPositive(int value) override;

    // ── Return type: int ─────────────────────────────────────────────────────
    Q_INVOKABLE int returnInt() override;
    Q_INVOKABLE int addInts(int a, int b) override;
    Q_INVOKABLE int stringLength(const QString& s) override;

    // ── Return type: QString ─────────────────────────────────────────────────
    Q_INVOKABLE QString returnString() override;
    Q_INVOKABLE QString echo(const QString& input) override;
    Q_INVOKABLE QString concat(const QString& a, const QString& b) override;

    // ── Return type: LogosResult ─────────────────────────────────────────────
    Q_INVOKABLE LogosResult successResult() override;
    Q_INVOKABLE LogosResult errorResult() override;
    Q_INVOKABLE LogosResult resultWithMap() override;
    Q_INVOKABLE LogosResult resultWithList() override;
    Q_INVOKABLE LogosResult validateInput(const QString& input) override;

    // ── Return type: QVariant ────────────────────────────────────────────────
    Q_INVOKABLE QVariant returnVariantInt() override;
    Q_INVOKABLE QVariant returnVariantString() override;
    Q_INVOKABLE QVariant returnVariantMap() override;
    Q_INVOKABLE QVariant returnVariantList() override;

    // ── Return type: QJsonArray ──────────────────────────────────────────────
    Q_INVOKABLE QJsonArray returnJsonArray() override;
    Q_INVOKABLE QJsonArray makeJsonArray(const QString& a, const QString& b) override;

    // ── Return type: QStringList ─────────────────────────────────────────────
    Q_INVOKABLE QStringList returnStringList() override;
    Q_INVOKABLE QStringList splitString(const QString& input) override;

    // ── Parameter types ──────────────────────────────────────────────────────
    Q_INVOKABLE int echoInt(int n) override;
    Q_INVOKABLE bool echoBool(bool b) override;
    Q_INVOKABLE QString joinStrings(const QStringList& list) override;
    Q_INVOKABLE int byteArraySize(const QByteArray& data) override;
    Q_INVOKABLE QString urlToString(const QUrl& url) override;

    // ── Argument counts 0–5 ──────────────────────────────────────────────────
    Q_INVOKABLE QString noArgs() override;
    Q_INVOKABLE QString oneArg(const QString& a) override;
    Q_INVOKABLE QString twoArgs(const QString& a, int b) override;
    Q_INVOKABLE QString threeArgs(const QString& a, int b, bool c) override;
    Q_INVOKABLE QString fourArgs(const QString& a, int b, bool c, const QString& d) override;
    Q_INVOKABLE QString fiveArgs(const QString& a, int b, bool c, const QString& d, int e) override;

    // ── Events ───────────────────────────────────────────────────────────────
    Q_INVOKABLE void emitTestEvent(const QString& data) override;
    Q_INVOKABLE void emitMultiArgEvent(const QString& name, int count) override;

signals:
    void eventResponse(const QString& eventName, const QVariantList& args);

private:
    LogosModules* logos = nullptr;
};

#endif // TEST_BASIC_MODULE_PLUGIN_H
