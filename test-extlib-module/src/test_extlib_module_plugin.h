#ifndef TEST_EXTLIB_MODULE_PLUGIN_H
#define TEST_EXTLIB_MODULE_PLUGIN_H

#include <QObject>
#include <QString>
#include "test_extlib_module_interface.h"

#include "lib/libstrutil.h"

class LogosAPI;

class TestExtlibModulePlugin : public QObject, public TestExtlibModuleInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID TestExtlibModuleInterface_iid FILE "metadata.json")
    Q_INTERFACES(TestExtlibModuleInterface PluginInterface)

public:
    explicit TestExtlibModulePlugin(QObject* parent = nullptr);
    ~TestExtlibModulePlugin() override;

    QString name() const override { return "test_extlib_module"; }
    QString version() const override { return "1.0.0"; }

    Q_INVOKABLE void initLogos(LogosAPI* api);

    Q_INVOKABLE QString reverseString(const QString& input) override;
    Q_INVOKABLE QString uppercaseString(const QString& input) override;
    Q_INVOKABLE QString lowercaseString(const QString& input) override;
    Q_INVOKABLE int countChars(const QString& input) override;
    Q_INVOKABLE int countChar(const QString& input, const QString& ch) override;
    Q_INVOKABLE QString libVersion() override;

signals:
    void eventResponse(const QString& eventName, const QVariantList& args);
};

#endif // TEST_EXTLIB_MODULE_PLUGIN_H
