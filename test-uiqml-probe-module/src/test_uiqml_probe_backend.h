#pragma once

#include "rep_test_uiqml_probe_source.h"
#include "logos_ui_plugin_context.h"

// SPIKE backend for the transport-choice probe. `type: ui_qml` +
// `interface: universal`, so the builder runs `logos-qt-generator --backend ui`
// around this class: we derive the repc SimpleSource (the QtRO view contract)
// and LogosUiPluginContext (onContextReady() + the Qt-typed modules()).
//
// The slots come in twins — a native one and a `*Json` one — that forward to
// the SAME test_fullapi_cpp method. See the .rep for what the two transports
// mean. The native bodies forward whatever QML already coerced; the QString
// bodies decode the text with the canonical codec and forward the typed value,
// so the two can be compared with nothing else varying.
class TestUiqmlProbeBackend : public TestUiqmlProbeSimpleSource,
                              public LogosUiPluginContext
{
public:
    TestUiqmlProbeBackend();

    // ── native transport ────────────────────────────────────────────────────
    void doEchoInt(qlonglong v) override;
    void doEchoUint(qulonglong v) override;
    void doEchoUintList(QVariantList v) override;
    void doEchoMap(QVariantMap v) override;
    void doEchoAny(QVariant v) override;
    void doEchoBytesNative(QByteArray v) override;

    // ── QString (canonical JSON text) transport ─────────────────────────────
    void doEchoIntJson(QString v) override;
    void doEchoUintJson(QString v) override;
    void doEchoBytesJson(QString v) override;
    void doEchoUintListJson(QString v) override;
    void doEchoMapJson(QString v) override;
    void doEchoAnyJson(QString v) override;

    // ── carried over from the original probe (native only) ──────────────────
    void doEchoBool(bool v) override;
    void doEchoStringList(QStringList v) override;
    void doEchoIntList(QVariantList v) override;
    void doEchoList(QVariantList v) override;

protected:
    void onContextReady() override;

private:
    // Publishes one answer, tagged with a sequence number. Every slot goes
    // through here: the view advances when the sequence moves, and a repeated
    // answer string would otherwise not move the property at all.
    void answer(const QString& payload);

    quint64 m_seq = 0;
};
