#include "test_ipc_new_api_impl.h"
#include <QDebug>
#include <QVariantMap>

void TestIpcNewApiImpl::onInit(LogosAPI* api)
{
    delete m_logos;
    m_logos = new LogosModules(api);
    m_basicClient = api->getClient("test_basic_module");
    m_extlibClient = api->getClient("test_extlib_module");
    qDebug() << "TestIpcNewApiImpl: LogosAPI initialized (new provider API)";
}

// ── Calls to test_basic_module ───────────────────────────────────────────────

QString TestIpcNewApiImpl::callBasicEcho(const QString& input)
{
    QVariant result = m_basicClient->invokeRemoteMethod(
        "test_basic_module", "echo", QVariant(input));
    qDebug() << "TestIpcNewApiImpl::callBasicEcho" << input << "->" << result;
    return result.toString();
}

int TestIpcNewApiImpl::callBasicAddInts(int a, int b)
{
    QVariant result = m_basicClient->invokeRemoteMethod(
        "test_basic_module", "addInts", QVariant(a), QVariant(b));
    qDebug() << "TestIpcNewApiImpl::callBasicAddInts" << a << b << "->" << result;
    return result.toInt();
}

bool TestIpcNewApiImpl::callBasicReturnTrue()
{
    QVariant result = m_basicClient->invokeRemoteMethod(
        "test_basic_module", "returnTrue");
    qDebug() << "TestIpcNewApiImpl::callBasicReturnTrue ->" << result;
    return result.toBool();
}

QString TestIpcNewApiImpl::callBasicNoArgs()
{
    QVariant result = m_basicClient->invokeRemoteMethod(
        "test_basic_module", "noArgs");
    qDebug() << "TestIpcNewApiImpl::callBasicNoArgs ->" << result;
    return result.toString();
}

QString TestIpcNewApiImpl::callBasicFiveArgs(const QString& a, int b, bool c, const QString& d, int e)
{
    QVariant result = m_basicClient->invokeRemoteMethod(
        "test_basic_module", "fiveArgs",
        QVariant(a), QVariant(b), QVariant(c), QVariant(d), QVariant(e));
    qDebug() << "TestIpcNewApiImpl::callBasicFiveArgs ->" << result;
    return result.toString();
}

LogosResult TestIpcNewApiImpl::callBasicSuccessResult()
{
    QVariant result = m_basicClient->invokeRemoteMethod(
        "test_basic_module", "successResult");
    if (result.canConvert<LogosResult>()) {
        return result.value<LogosResult>();
    }
    return {true, result, QVariant()};
}

LogosResult TestIpcNewApiImpl::callBasicErrorResult()
{
    QVariant result = m_basicClient->invokeRemoteMethod(
        "test_basic_module", "errorResult");
    if (result.canConvert<LogosResult>()) {
        return result.value<LogosResult>();
    }
    return {false, QVariant(), result};
}

QString TestIpcNewApiImpl::callBasicResultMapField(const QString& key)
{
    QVariant result = m_basicClient->invokeRemoteMethod(
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

// ── Calls to test_extlib_module ──────────────────────────────────────────────

QString TestIpcNewApiImpl::callExtlibReverse(const QString& input)
{
    QVariant result = m_extlibClient->invokeRemoteMethod(
        "test_extlib_module", "reverseString", QVariant(input));
    qDebug() << "TestIpcNewApiImpl::callExtlibReverse" << input << "->" << result;
    return result.toString();
}

QString TestIpcNewApiImpl::callExtlibUppercase(const QString& input)
{
    QVariant result = m_extlibClient->invokeRemoteMethod(
        "test_extlib_module", "uppercaseString", QVariant(input));
    qDebug() << "TestIpcNewApiImpl::callExtlibUppercase" << input << "->" << result;
    return result.toString();
}

int TestIpcNewApiImpl::callExtlibCountChars(const QString& input)
{
    QVariant result = m_extlibClient->invokeRemoteMethod(
        "test_extlib_module", "countChars", QVariant(input));
    qDebug() << "TestIpcNewApiImpl::callExtlibCountChars" << input << "->" << result;
    return result.toInt();
}

// ── Cross-module chaining ────────────────────────────────────────────────────

QString TestIpcNewApiImpl::chainEchoThenReverse(const QString& input)
{
    QVariant echoed = m_basicClient->invokeRemoteMethod(
        "test_basic_module", "echo", QVariant(input));
    QVariant reversed = m_extlibClient->invokeRemoteMethod(
        "test_extlib_module", "reverseString", echoed);
    qDebug() << "TestIpcNewApiImpl::chainEchoThenReverse" << input << "->" << reversed;
    return reversed.toString();
}

QString TestIpcNewApiImpl::chainUppercaseThenConcat(const QString& a, const QString& b)
{
    QVariant upperA = m_extlibClient->invokeRemoteMethod(
        "test_extlib_module", "uppercaseString", QVariant(a));
    QVariant upperB = m_extlibClient->invokeRemoteMethod(
        "test_extlib_module", "uppercaseString", QVariant(b));
    QVariant result = m_basicClient->invokeRemoteMethod(
        "test_basic_module", "concat", upperA, upperB);
    qDebug() << "TestIpcNewApiImpl::chainUppercaseThenConcat" << a << b << "->" << result;
    return result.toString();
}

// ── Generated type-safe wrappers (LogosModules) ─────────────────────────────

QString TestIpcNewApiImpl::wrapperBasicEcho(const QString& input)
{
    if (!m_logos) return QString();
    QString result = m_logos->test_basic_module.echo(input);
    qDebug() << "TestIpcNewApiImpl::wrapperBasicEcho" << input << "->" << result;
    return result;
}

QString TestIpcNewApiImpl::wrapperExtlibReverse(const QString& input)
{
    if (!m_logos) return QString();
    QString result = m_logos->test_extlib_module.reverseString(input);
    qDebug() << "TestIpcNewApiImpl::wrapperExtlibReverse" << input << "->" << result;
    return result;
}

// ── Events ───────────────────────────────────────────────────────────────────

void TestIpcNewApiImpl::triggerBasicEvent(const QString& data)
{
    m_basicClient->invokeRemoteMethod(
        "test_basic_module", "emitTestEvent", QVariant(data));
    emitEvent("triggeredBasicEvent", QVariantList() << data);
    qDebug() << "TestIpcNewApiImpl::triggerBasicEvent" << data;
}
