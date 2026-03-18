#ifndef TEST_IPC_NEW_API_LOADER_H
#define TEST_IPC_NEW_API_LOADER_H

#include <QObject>
#include "interface.h"
#include "logos_provider_object.h"
#include "test_ipc_new_api_impl.h"

class TestIpcNewApiLoader : public QObject, public PluginInterface, public LogosProviderPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LogosProviderPlugin_iid FILE "metadata.json")
    Q_INTERFACES(PluginInterface LogosProviderPlugin)

public:
    QString name() const override { return "test_ipc_new_api_module"; }
    QString version() const override { return "1.0.0"; }
    LogosProviderObject* createProviderObject() override { return new TestIpcNewApiImpl(); }
};

#endif // TEST_IPC_NEW_API_LOADER_H
