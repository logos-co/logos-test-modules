#ifndef DUMMY_MODULE_000000_LOADER_H
#define DUMMY_MODULE_000000_LOADER_H

#include <QObject>
#include "interface.h"
#include "logos_provider_object.h"
#include "dummy_module_000000_impl.h"

class DummyModule000000Loader : public QObject, public PluginInterface, public LogosProviderPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LogosProviderPlugin_iid FILE "metadata.json")
    Q_INTERFACES(PluginInterface LogosProviderPlugin)

public:
    QString name() const override { return "dummy_module_000000"; }
    QString version() const override { return "1.0.0"; }
    LogosProviderObject* createProviderObject() override { return new DummyModule000000Impl(); }
};

#endif // DUMMY_MODULE_000000_LOADER_H
