#ifndef TEST_BASIC_MODULE_INTERFACE_H
#define TEST_BASIC_MODULE_INTERFACE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QUrl>
#include <QVariant>
#include <QJsonArray>
#include "interface.h"
#include "logos_types.h"

class TestBasicModuleInterface : public PluginInterface
{
public:
    virtual ~TestBasicModuleInterface() = default;

    // ── Return type: void ────────────────────────────────────────────────────
    Q_INVOKABLE virtual void doNothing() = 0;
    Q_INVOKABLE virtual void doNothingWithArgs(const QString& a, int b) = 0;

    // ── Return type: bool ────────────────────────────────────────────────────
    Q_INVOKABLE virtual bool returnTrue() = 0;
    Q_INVOKABLE virtual bool returnFalse() = 0;
    Q_INVOKABLE virtual bool isPositive(int value) = 0;

    // ── Return type: int ─────────────────────────────────────────────────────
    Q_INVOKABLE virtual int returnInt() = 0;
    Q_INVOKABLE virtual int addInts(int a, int b) = 0;
    Q_INVOKABLE virtual int stringLength(const QString& s) = 0;

    // ── Return type: QString ─────────────────────────────────────────────────
    Q_INVOKABLE virtual QString returnString() = 0;
    Q_INVOKABLE virtual QString echo(const QString& input) = 0;
    Q_INVOKABLE virtual QString concat(const QString& a, const QString& b) = 0;

    // ── Return type: LogosResult ─────────────────────────────────────────────
    Q_INVOKABLE virtual LogosResult successResult() = 0;
    Q_INVOKABLE virtual LogosResult errorResult() = 0;
    Q_INVOKABLE virtual LogosResult resultWithMap() = 0;
    Q_INVOKABLE virtual LogosResult resultWithList() = 0;
    Q_INVOKABLE virtual LogosResult validateInput(const QString& input) = 0;

    // ── Return type: QVariant ────────────────────────────────────────────────
    Q_INVOKABLE virtual QVariant returnVariantInt() = 0;
    Q_INVOKABLE virtual QVariant returnVariantString() = 0;
    Q_INVOKABLE virtual QVariant returnVariantMap() = 0;
    Q_INVOKABLE virtual QVariant returnVariantList() = 0;

    // ── Return type: QJsonArray ──────────────────────────────────────────────
    Q_INVOKABLE virtual QJsonArray returnJsonArray() = 0;
    Q_INVOKABLE virtual QJsonArray makeJsonArray(const QString& a, const QString& b) = 0;

    // ── Return type: QStringList ─────────────────────────────────────────────
    Q_INVOKABLE virtual QStringList returnStringList() = 0;
    Q_INVOKABLE virtual QStringList splitString(const QString& input) = 0;

    // ── Parameter type: int ──────────────────────────────────────────────────
    Q_INVOKABLE virtual int echoInt(int n) = 0;

    // ── Parameter type: bool ─────────────────────────────────────────────────
    Q_INVOKABLE virtual bool echoBool(bool b) = 0;

    // ── Parameter type: QStringList ──────────────────────────────────────────
    Q_INVOKABLE virtual QString joinStrings(const QStringList& list) = 0;

    // ── Parameter type: QByteArray ───────────────────────────────────────────
    Q_INVOKABLE virtual int byteArraySize(const QByteArray& data) = 0;

    // ── Parameter type: QUrl ─────────────────────────────────────────────────
    Q_INVOKABLE virtual QString urlToString(const QUrl& url) = 0;

    // ── Argument counts 0–5 ──────────────────────────────────────────────────
    Q_INVOKABLE virtual QString noArgs() = 0;
    Q_INVOKABLE virtual QString oneArg(const QString& a) = 0;
    Q_INVOKABLE virtual QString twoArgs(const QString& a, int b) = 0;
    Q_INVOKABLE virtual QString threeArgs(const QString& a, int b, bool c) = 0;
    Q_INVOKABLE virtual QString fourArgs(const QString& a, int b, bool c, const QString& d) = 0;
    Q_INVOKABLE virtual QString fiveArgs(const QString& a, int b, bool c, const QString& d, int e) = 0;

    // ── Events ───────────────────────────────────────────────────────────────
    Q_INVOKABLE virtual void emitTestEvent(const QString& data) = 0;
    Q_INVOKABLE virtual void emitMultiArgEvent(const QString& name, int count) = 0;

    // ── Async helpers ─────────────────────────────────────────────────────────
    // Returns value after delayMs milliseconds — used to exercise async timeouts
    Q_INVOKABLE virtual QString echoWithDelay(const QString& value, int delayMs) = 0;
};

#define TestBasicModuleInterface_iid "org.logos.TestBasicModuleInterface"
Q_DECLARE_INTERFACE(TestBasicModuleInterface, TestBasicModuleInterface_iid)

#endif // TEST_BASIC_MODULE_INTERFACE_H
