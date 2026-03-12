#include "test_extlib_module_plugin.h"
#include "logos_api.h"
#include <QDebug>
#include <QByteArray>

TestExtlibModulePlugin::TestExtlibModulePlugin(QObject* parent)
    : QObject(parent)
{
    qDebug() << "TestExtlibModulePlugin: created";
}

TestExtlibModulePlugin::~TestExtlibModulePlugin()
{
    qDebug() << "TestExtlibModulePlugin: destroyed";
}

void TestExtlibModulePlugin::initLogos(LogosAPI* api)
{
    logosAPI = api;
    qDebug() << "TestExtlibModulePlugin: LogosAPI initialized";
}

QString TestExtlibModulePlugin::reverseString(const QString& input)
{
    QByteArray utf8 = input.toUtf8();
    QByteArray buf(utf8.size() + 1, '\0');
    strutil_reverse(utf8.constData(), buf.data());
    QString result = QString::fromUtf8(buf.constData());
    qDebug() << "TestExtlibModulePlugin::reverseString" << input << "->" << result;
    return result;
}

QString TestExtlibModulePlugin::uppercaseString(const QString& input)
{
    QByteArray utf8 = input.toUtf8();
    QByteArray buf(utf8.size() + 1, '\0');
    strutil_uppercase(utf8.constData(), buf.data());
    QString result = QString::fromUtf8(buf.constData());
    qDebug() << "TestExtlibModulePlugin::uppercaseString" << input << "->" << result;
    return result;
}

QString TestExtlibModulePlugin::lowercaseString(const QString& input)
{
    QByteArray utf8 = input.toUtf8();
    QByteArray buf(utf8.size() + 1, '\0');
    strutil_lowercase(utf8.constData(), buf.data());
    QString result = QString::fromUtf8(buf.constData());
    qDebug() << "TestExtlibModulePlugin::lowercaseString" << input << "->" << result;
    return result;
}

int TestExtlibModulePlugin::countChars(const QString& input)
{
    int result = strutil_count_chars(input.toUtf8().constData());
    qDebug() << "TestExtlibModulePlugin::countChars" << input << "=" << result;
    return result;
}

int TestExtlibModulePlugin::countChar(const QString& input, const QString& ch)
{
    char c = ch.isEmpty() ? '\0' : ch.at(0).toLatin1();
    int result = strutil_count_char(input.toUtf8().constData(), c);
    qDebug() << "TestExtlibModulePlugin::countChar" << input << ch << "=" << result;
    return result;
}

QString TestExtlibModulePlugin::libVersion()
{
    const char* ver = strutil_version();
    QString result = QString::fromUtf8(ver);
    qDebug() << "TestExtlibModulePlugin::libVersion" << result;
    return result;
}
