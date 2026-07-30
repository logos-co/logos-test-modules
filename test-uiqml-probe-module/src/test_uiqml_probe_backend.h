#pragma once

#include "rep_test_uiqml_probe_source.h"
#include "logos_ui_plugin_context.h"

// THROWAWAY PROBE backend. `type: ui_qml` + `interface: universal`, so the
// builder runs `logos-qt-generator --backend ui` around this class: we derive
// the repc SimpleSource (the QtRO view contract) and LogosUiPluginContext
// (onContextReady() + the Qt-typed modules() accessors).
//
// Each slot forwards to test_fullapi_cpp through the generated Qt-typed
// wrapper and records the answer in the lastResult PROP.
class TestUiqmlProbeBackend : public TestUiqmlProbeSimpleSource,
                              public LogosUiPluginContext
{
public:
    void doEchoUint(qulonglong v) override;
    void doEchoInt(qlonglong v) override;
    void doEchoBool(bool v) override;
    void doEchoStringList(QStringList v) override;
    void doEchoUintList(QVariantList v) override;
    void doEchoIntList(QVariantList v) override;
    void doEchoList(QVariantList v) override;
    void doEchoMap(QVariantMap v) override;

protected:
    void onContextReady() override;
};
