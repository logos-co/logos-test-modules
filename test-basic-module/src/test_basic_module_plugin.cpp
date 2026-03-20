#include "test_basic_module_plugin.h"
#include "logos_api.h"
#include "logos_api_client.h"
#include <QDebug>
#include <QJsonValue>
#include <QVariantMap>
#include <QVariantList>
#include <QThread>

TestBasicModulePlugin::TestBasicModulePlugin(QObject* parent)
    : QObject(parent)
{
    qDebug() << "TestBasicModulePlugin: created";
}

TestBasicModulePlugin::~TestBasicModulePlugin()
{
    qDebug() << "TestBasicModulePlugin: destroyed";
    delete logos;
}

void TestBasicModulePlugin::initLogos(LogosAPI* logosAPIInstance)
{
    delete logos;
    logos = nullptr;
    logosAPI = logosAPIInstance;
    if (logosAPI) {
        logos = new LogosModules(logosAPI);
    }
    qDebug() << "TestBasicModulePlugin: LogosAPI initialized";
}

// ── Return type: void ────────────────────────────────────────────────────────

void TestBasicModulePlugin::doNothing()
{
    qDebug() << "TestBasicModulePlugin::doNothing";
}

void TestBasicModulePlugin::doNothingWithArgs(const QString& a, int b)
{
    qDebug() << "TestBasicModulePlugin::doNothingWithArgs" << a << b;
}

// ── Return type: bool ────────────────────────────────────────────────────────

bool TestBasicModulePlugin::returnTrue()
{
    return true;
}

bool TestBasicModulePlugin::returnFalse()
{
    return false;
}

bool TestBasicModulePlugin::isPositive(int value)
{
    return value > 0;
}

// ── Return type: int ─────────────────────────────────────────────────────────

int TestBasicModulePlugin::returnInt()
{
    return 42;
}

int TestBasicModulePlugin::addInts(int a, int b)
{
    return a + b;
}

int TestBasicModulePlugin::stringLength(const QString& s)
{
    return s.length();
}

// ── Return type: QString ─────────────────────────────────────────────────────

QString TestBasicModulePlugin::returnString()
{
    return QStringLiteral("test_basic_module");
}

QString TestBasicModulePlugin::echo(const QString& input)
{
    return input;
}

QString TestBasicModulePlugin::concat(const QString& a, const QString& b)
{
    return a + b;
}

// ── Return type: LogosResult ─────────────────────────────────────────────────

LogosResult TestBasicModulePlugin::successResult()
{
    return {true, QVariant(QStringLiteral("operation succeeded")), QVariant()};
}

LogosResult TestBasicModulePlugin::errorResult()
{
    return {false, QVariant(), QVariant(QStringLiteral("deliberate error for testing"))};
}

LogosResult TestBasicModulePlugin::resultWithMap()
{
    QVariantMap map;
    map["name"] = "test";
    map["count"] = 42;
    map["active"] = true;
    return {true, map, QVariant()};
}

LogosResult TestBasicModulePlugin::resultWithList()
{
    QVariantList list;
    QVariantMap item1;
    item1["id"] = 1;
    item1["label"] = "first";
    QVariantMap item2;
    item2["id"] = 2;
    item2["label"] = "second";
    list.append(item1);
    list.append(item2);
    return {true, list, QVariant()};
}

LogosResult TestBasicModulePlugin::validateInput(const QString& input)
{
    if (input.isEmpty()) {
        return {false, QVariant(), QVariant(QStringLiteral("input cannot be empty"))};
    }
    QVariantMap data;
    data["input"] = input;
    data["length"] = input.length();
    return {true, data, QVariant()};
}

// ── Return type: QVariant ────────────────────────────────────────────────────

QVariant TestBasicModulePlugin::returnVariantInt()
{
    return QVariant(99);
}

QVariant TestBasicModulePlugin::returnVariantString()
{
    return QVariant(QStringLiteral("variant_string"));
}

QVariant TestBasicModulePlugin::returnVariantMap()
{
    QVariantMap map;
    map["key"] = "value";
    map["number"] = 7;
    return QVariant(map);
}

QVariant TestBasicModulePlugin::returnVariantList()
{
    QVariantList list;
    list.append("alpha");
    list.append("beta");
    list.append("gamma");
    return QVariant(list);
}

// ── Return type: QJsonArray ──────────────────────────────────────────────────

QJsonArray TestBasicModulePlugin::returnJsonArray()
{
    QJsonArray arr;
    arr.append(QJsonValue(1));
    arr.append(QJsonValue(2));
    arr.append(QJsonValue(3));
    return arr;
}

QJsonArray TestBasicModulePlugin::makeJsonArray(const QString& a, const QString& b)
{
    QJsonArray arr;
    arr.append(QJsonValue(a));
    arr.append(QJsonValue(b));
    return arr;
}

// ── Return type: QStringList ─────────────────────────────────────────────────

QStringList TestBasicModulePlugin::returnStringList()
{
    return QStringList() << "one" << "two" << "three";
}

QStringList TestBasicModulePlugin::splitString(const QString& input)
{
    return input.split(",", Qt::SkipEmptyParts);
}

// ── Parameter types ──────────────────────────────────────────────────────────

int TestBasicModulePlugin::echoInt(int n)
{
    return n;
}

bool TestBasicModulePlugin::echoBool(bool b)
{
    return b;
}

QString TestBasicModulePlugin::joinStrings(const QStringList& list)
{
    return list.join(", ");
}

int TestBasicModulePlugin::byteArraySize(const QByteArray& data)
{
    return data.size();
}

QString TestBasicModulePlugin::urlToString(const QUrl& url)
{
    return url.toString();
}

// ── Argument counts 0–5 ──────────────────────────────────────────────────────

QString TestBasicModulePlugin::noArgs()
{
    return QStringLiteral("noArgs()");
}

QString TestBasicModulePlugin::oneArg(const QString& a)
{
    return QStringLiteral("oneArg(%1)").arg(a);
}

QString TestBasicModulePlugin::twoArgs(const QString& a, int b)
{
    return QStringLiteral("twoArgs(%1, %2)").arg(a).arg(b);
}

QString TestBasicModulePlugin::threeArgs(const QString& a, int b, bool c)
{
    return QStringLiteral("threeArgs(%1, %2, %3)").arg(a).arg(b).arg(c ? "true" : "false");
}

QString TestBasicModulePlugin::fourArgs(const QString& a, int b, bool c, const QString& d)
{
    return QStringLiteral("fourArgs(%1, %2, %3, %4)").arg(a).arg(b).arg(c ? "true" : "false").arg(d);
}

QString TestBasicModulePlugin::fiveArgs(const QString& a, int b, bool c, const QString& d, int e)
{
    return QStringLiteral("fiveArgs(%1, %2, %3, %4, %5)").arg(a).arg(b).arg(c ? "true" : "false").arg(d).arg(e);
}

// ── Events ───────────────────────────────────────────────────────────────────

void TestBasicModulePlugin::emitTestEvent(const QString& data)
{
    emit eventResponse("testEvent", QVariantList() << data);
}

void TestBasicModulePlugin::emitMultiArgEvent(const QString& name, int count)
{
    emit eventResponse("multiArgEvent", QVariantList() << name << count);
}

// ── Async helpers ─────────────────────────────────────────────────────────────

QString TestBasicModulePlugin::echoWithDelay(const QString& value, int delayMs)
{
    qDebug() << "TestBasicModulePlugin::echoWithDelay" << value << "delay:" << delayMs;
    QThread::msleep(delayMs);
    return value;
}
