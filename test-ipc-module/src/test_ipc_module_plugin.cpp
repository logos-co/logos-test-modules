#include "test_ipc_module_plugin.h"
#include "logos_api.h"
#include "logos_api_client.h"
#include <QDebug>
#include <QEventLoop>
#include <QVariantMap>

TestIpcModulePlugin::TestIpcModulePlugin(QObject* parent)
    : QObject(parent)
{
    qDebug() << "TestIpcModulePlugin: created";
}

TestIpcModulePlugin::~TestIpcModulePlugin()
{
    qDebug() << "TestIpcModulePlugin: destroyed";
    delete logos;
}

void TestIpcModulePlugin::initLogos(LogosAPI* logosAPIInstance)
{
    delete logos;
    logos = nullptr;
    logosAPI = logosAPIInstance;
    if (logosAPI) {
        logos = new LogosModules(logosAPI);
        basicClient = logosAPI->getClient("test_basic_module");
        extlibClient = logosAPI->getClient("test_extlib_module");
    }
    qDebug() << "TestIpcModulePlugin: LogosAPI initialized";
}

// ── Raw invokeRemoteMethod calls to test_basic_module ────────────────────────

QString TestIpcModulePlugin::callBasicEcho(const QString& input)
{
    QVariant result = basicClient->invokeRemoteMethod(
        "test_basic_module", "echo", QVariant(input));
    qDebug() << "TestIpcModulePlugin::callBasicEcho" << input << "->" << result;
    return result.toString();
}

int TestIpcModulePlugin::callBasicAddInts(int a, int b)
{
    QVariant result = basicClient->invokeRemoteMethod(
        "test_basic_module", "addInts", QVariant(a), QVariant(b));
    qDebug() << "TestIpcModulePlugin::callBasicAddInts" << a << b << "->" << result;
    return result.toInt();
}

bool TestIpcModulePlugin::callBasicReturnTrue()
{
    QVariant result = basicClient->invokeRemoteMethod(
        "test_basic_module", "returnTrue");
    qDebug() << "TestIpcModulePlugin::callBasicReturnTrue ->" << result;
    return result.toBool();
}

QString TestIpcModulePlugin::callBasicNoArgs()
{
    QVariant result = basicClient->invokeRemoteMethod(
        "test_basic_module", "noArgs");
    qDebug() << "TestIpcModulePlugin::callBasicNoArgs ->" << result;
    return result.toString();
}

QString TestIpcModulePlugin::callBasicFiveArgs(const QString& a, int b, bool c, const QString& d, int e)
{
    QVariant result = basicClient->invokeRemoteMethod(
        "test_basic_module", "fiveArgs",
        QVariant(a), QVariant(b), QVariant(c), QVariant(d), QVariant(e));
    qDebug() << "TestIpcModulePlugin::callBasicFiveArgs ->" << result;
    return result.toString();
}

LogosResult TestIpcModulePlugin::callBasicSuccessResult()
{
    QVariant result = basicClient->invokeRemoteMethod(
        "test_basic_module", "successResult");
    if (result.canConvert<LogosResult>()) {
        return result.value<LogosResult>();
    }
    return {true, result, QVariant()};
}

LogosResult TestIpcModulePlugin::callBasicErrorResult()
{
    QVariant result = basicClient->invokeRemoteMethod(
        "test_basic_module", "errorResult");
    if (result.canConvert<LogosResult>()) {
        return result.value<LogosResult>();
    }
    return {false, QVariant(), result};
}

QString TestIpcModulePlugin::callBasicResultMapField(const QString& key)
{
    QVariant result = basicClient->invokeRemoteMethod(
        "test_basic_module", "resultWithMap");
    if (result.canConvert<LogosResult>()) {
        LogosResult lr = result.value<LogosResult>();
        if (lr.success) {
            QVariantMap map = lr.getValue<QVariantMap>();
            return map.value(key).toString();
        }
    }
    return QString();
}

// ── Raw invokeRemoteMethod calls to test_extlib_module ───────────────────────

QString TestIpcModulePlugin::callExtlibReverse(const QString& input)
{
    QVariant result = extlibClient->invokeRemoteMethod(
        "test_extlib_module", "reverseString", QVariant(input));
    qDebug() << "TestIpcModulePlugin::callExtlibReverse" << input << "->" << result;
    return result.toString();
}

QString TestIpcModulePlugin::callExtlibUppercase(const QString& input)
{
    QVariant result = extlibClient->invokeRemoteMethod(
        "test_extlib_module", "uppercaseString", QVariant(input));
    qDebug() << "TestIpcModulePlugin::callExtlibUppercase" << input << "->" << result;
    return result.toString();
}

int TestIpcModulePlugin::callExtlibCountChars(const QString& input)
{
    QVariant result = extlibClient->invokeRemoteMethod(
        "test_extlib_module", "countChars", QVariant(input));
    qDebug() << "TestIpcModulePlugin::callExtlibCountChars" << input << "->" << result;
    return result.toInt();
}

// ── Cross-module chaining ────────────────────────────────────────────────────

QString TestIpcModulePlugin::chainEchoThenReverse(const QString& input)
{
    QVariant echoed = basicClient->invokeRemoteMethod(
        "test_basic_module", "echo", QVariant(input));
    QVariant reversed = extlibClient->invokeRemoteMethod(
        "test_extlib_module", "reverseString", echoed);
    qDebug() << "TestIpcModulePlugin::chainEchoThenReverse" << input << "->" << reversed;
    return reversed.toString();
}

QString TestIpcModulePlugin::chainUppercaseThenConcat(const QString& a, const QString& b)
{
    QVariant upperA = extlibClient->invokeRemoteMethod(
        "test_extlib_module", "uppercaseString", QVariant(a));
    QVariant upperB = extlibClient->invokeRemoteMethod(
        "test_extlib_module", "uppercaseString", QVariant(b));
    QVariant result = basicClient->invokeRemoteMethod(
        "test_basic_module", "concat", upperA, upperB);
    qDebug() << "TestIpcModulePlugin::chainUppercaseThenConcat" << a << b << "->" << result;
    return result.toString();
}

// ── Generated type-safe wrappers (LogosModules) ─────────────────────────────

QString TestIpcModulePlugin::wrapperBasicEcho(const QString& input)
{
    if (!logos) return QString();
    QString result = logos->test_basic_module.echo(input);
    qDebug() << "TestIpcModulePlugin::wrapperBasicEcho" << input << "->" << result;
    return result;
}

QString TestIpcModulePlugin::wrapperExtlibReverse(const QString& input)
{
    if (!logos) return QString();
    QString result = logos->test_extlib_module.reverseString(input);
    qDebug() << "TestIpcModulePlugin::wrapperExtlibReverse" << input << "->" << result;
    return result;
}

// ── Events ───────────────────────────────────────────────────────────────────

void TestIpcModulePlugin::triggerBasicEvent(const QString& data)
{
    basicClient->invokeRemoteMethod(
        "test_basic_module", "emitTestEvent", QVariant(data));
    emit eventResponse("triggeredBasicEvent", QVariantList() << data);
    qDebug() << "TestIpcModulePlugin::triggerBasicEvent" << data;
}

// ── Async calls ──────────────────────────────────────────────────────────────
// Each method uses a QEventLoop to block until the async callback fires,
// making the result observable by logoscore's synchronous -c interface.

QString TestIpcModulePlugin::asyncCallBasicEcho(const QString& input)
{
    QString result;
    QEventLoop loop;
    basicClient->invokeRemoteMethodAsync("test_basic_module", "echo", QVariant(input),
        [&result, &loop](QVariant v) {
            result = v.toString();
            loop.quit();
        });
    loop.exec();
    qDebug() << "TestIpcModulePlugin::asyncCallBasicEcho" << input << "->" << result;
    return result;
}

int TestIpcModulePlugin::asyncCallBasicAddInts(int a, int b)
{
    int result = 0;
    QEventLoop loop;
    basicClient->invokeRemoteMethodAsync("test_basic_module", "addInts",
        QVariantList{QVariant(a), QVariant(b)},
        [&result, &loop](QVariant v) {
            result = v.toInt();
            loop.quit();
        });
    loop.exec();
    qDebug() << "TestIpcModulePlugin::asyncCallBasicAddInts" << a << b << "->" << result;
    return result;
}

QString TestIpcModulePlugin::asyncCallExtlibReverse(const QString& input)
{
    QString result;
    QEventLoop loop;
    extlibClient->invokeRemoteMethodAsync("test_extlib_module", "reverseString", QVariant(input),
        [&result, &loop](QVariant v) {
            result = v.toString();
            loop.quit();
        });
    loop.exec();
    qDebug() << "TestIpcModulePlugin::asyncCallExtlibReverse" << input << "->" << result;
    return result;
}

QString TestIpcModulePlugin::asyncWrapperBasicEcho(const QString& input)
{
    if (!logos) return QString();
    QString result;
    QEventLoop loop;
    logos->test_basic_module.echoAsync(input, [&result, &loop](QString v) {
        result = v;
        loop.quit();
    });
    loop.exec();
    qDebug() << "TestIpcModulePlugin::asyncWrapperBasicEcho" << input << "->" << result;
    return result;
}
