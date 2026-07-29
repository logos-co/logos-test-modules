#ifndef TEST_QTBIND_PROBE_LOADER_H
#define TEST_QTBIND_PROBE_LOADER_H

#include <QObject>
#include "interface.h"
#include "logos_provider_object.h"
#include "test_qtbind_probe_impl.h"

class TestQtbindProbeLoader : public QObject, public PluginInterface, public LogosProviderPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LogosProviderPlugin_iid FILE "metadata.json")
    Q_INTERFACES(PluginInterface LogosProviderPlugin)

public:
    QString name() const override { return "test_qtbind_probe"; }
    QString version() const override { return "1.0.0"; }
    LogosProviderObject* createProviderObject() override { return new TestQtbindProbeImpl(); }
};

#endif // TEST_QTBIND_PROBE_LOADER_H
