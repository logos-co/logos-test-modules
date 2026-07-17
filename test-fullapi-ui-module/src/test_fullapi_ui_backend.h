#pragma once

#include "rep_test_fullapi_ui_source.h"
#include "logos_ui_plugin_context.h"

// The hand-written UI backend (universal authoring model). You write only this
// class + the .rep contract; the *Plugin / *Interface glue (Q_PLUGIN_METADATA,
// initLogos wiring, QtRO registration) is generated around it.
//
// It derives:
//   * TestFullapiUiSimpleSource — from the .rep: implement its SLOTs, feed its
//     PROPs (setStatus / setLastEvent / setBoundTarget), which sync to QML.
//   * LogosUiPluginContext — gives onContextReady() + modules(), whose Qt-typed
//     bind_full_api(name) wrapper drives the full_api interface dependency.
class TestFullapiUiBackend : public TestFullapiUiSimpleSource,
                             public LogosUiPluginContext
{
public:
    void bindTo(QString name) override;
    void runMethods() override;
    void fireEvents() override;

protected:
    void onContextReady() override;

private:
    // Subscribe to the bound target's typed events; each updates the lastEvent
    // PROP so the QML view (and the doctest) can observe event delivery.
    void subscribe();
};
