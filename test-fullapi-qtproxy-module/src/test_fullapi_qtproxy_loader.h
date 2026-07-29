#ifndef TEST_FULLAPI_QTPROXY_LOADER_H
#define TEST_FULLAPI_QTPROXY_LOADER_H

#include <QObject>
#include "interface.h"
#include "logos_provider_object.h"
#include "test_fullapi_qtproxy_impl.h"

class TestFullapiQtproxyLoader : public QObject, public PluginInterface, public LogosProviderPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LogosProviderPlugin_iid FILE "metadata.json")
    Q_INTERFACES(PluginInterface LogosProviderPlugin)

public:
    QString name() const override { return "test_fullapi_qtproxy"; }
    QString version() const override { return "1.0.0"; }
    LogosProviderObject* createProviderObject() override { return new TestFullapiQtproxyImpl(); }
};

#endif // TEST_FULLAPI_QTPROXY_LOADER_H
