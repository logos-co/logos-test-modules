#ifndef TEST_IPC_NEW_API_LOADER_H
#define TEST_IPC_NEW_API_LOADER_H

#include <QObject>
#include <QtPlugin>
#include "interface.h"
#include "logos_native_provider.h"
#include "test_ipc_new_api_impl.h"

Q_DECLARE_INTERFACE(NativeProviderPlugin, NativeProviderPlugin_iid)

class TestIpcNewApiLoader : public QObject, public PluginInterface, public NativeProviderPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID NativeProviderPlugin_iid FILE "metadata.json")
    Q_INTERFACES(PluginInterface NativeProviderPlugin)

public:
    QString name() const override { return "test_ipc_new_api_module"; }
    QString version() const override { return "1.0.0"; }
    NativeProviderObject* createNativeProviderObject() override { return new TestIpcNewApiImpl(); }
};

#endif // TEST_IPC_NEW_API_LOADER_H
