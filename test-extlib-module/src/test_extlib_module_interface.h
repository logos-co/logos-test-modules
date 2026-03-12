#ifndef TEST_EXTLIB_MODULE_INTERFACE_H
#define TEST_EXTLIB_MODULE_INTERFACE_H

#include <QObject>
#include <QString>
#include "interface.h"

class TestExtlibModuleInterface : public PluginInterface
{
public:
    virtual ~TestExtlibModuleInterface() = default;

    Q_INVOKABLE virtual QString reverseString(const QString& input) = 0;
    Q_INVOKABLE virtual QString uppercaseString(const QString& input) = 0;
    Q_INVOKABLE virtual QString lowercaseString(const QString& input) = 0;
    Q_INVOKABLE virtual int countChars(const QString& input) = 0;
    Q_INVOKABLE virtual int countChar(const QString& input, const QString& ch) = 0;
    Q_INVOKABLE virtual QString libVersion() = 0;
};

#define TestExtlibModuleInterface_iid "org.logos.TestExtlibModuleInterface"
Q_DECLARE_INTERFACE(TestExtlibModuleInterface, TestExtlibModuleInterface_iid)

#endif // TEST_EXTLIB_MODULE_INTERFACE_H
