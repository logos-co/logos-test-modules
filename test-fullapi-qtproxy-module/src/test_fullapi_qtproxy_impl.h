#ifndef TEST_FULLAPI_QTPROXY_IMPL_H
#define TEST_FULLAPI_QTPROXY_IMPL_H

// ─────────────────────────────────────────────────────────────────────────────
// test_fullapi_qtproxy — the QT-TYPED consumer of the full_api contract.
//
// The conformance matrix has two providers but only ever had one consumer
// surface reachable from a driver (`py`, via the logoscore CLI). The two
// existing proxies do not add one: `interface: "universal"` selects apiStyle=lp
// and `interface: "cdylib"` selects the Rust client, so BOTH bypass the Qt
// generated wrappers entirely.
//
// This module is `type: core` with NO `interface` key. That combination selects
// apiStyle=qt (see logos-module-builder lib/mkLogosModule.nix, apiStyleCmakeFlags
// — lp is chosen only for universal non-ui_qml), so `modules().bind_full_api()`
// hands back a Qt-TYPED `FullApi`: QString / QByteArray / qlonglong / qulonglong
// / QVariantList / QVariantMap / LogosResult. Two things only this path can
// reach:
//
//   * registry entry M3 — a one-key `_bytes` map reinterpreted as bytes happens
//     in logos-protocol's logos_json_convert.cpp `nlohmannToQVariant`, which is
//     on the way to a Qt consumer and nowhere else.
//   * the generated ASYNC return table converts with `qvariant_cast<T>(v)` where
//     the SYNC one uses `_result.toT()` — different semantics for the same LIDL
//     type. syncProbe() / probeAsync() + getAsyncProbe() render the SAME inputs
//     through both so a driver can diff them directly.
//
// The forwarding surface mirrors test-fullapi-proxy-module-cpp 1:1 (33 methods,
// 15 events) so the matrix replays cases.json through this consumer unchanged:
//   logoscore call test_fullapi_qtproxy echoInt 42
//
// Every declaration below is scanned by logos-cpp-generator --provider-header,
// whose regex needs the whole declaration on ONE line ending in `;`. Do not wrap.
// ─────────────────────────────────────────────────────────────────────────────

#include "logos_provider_object.h"
#include "logos_api.h"
#include "logos_api_client.h"
#include "logos_sdk.h"
#include "logos_types.h"

#include <QByteArray>
#include <QMutex>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

class TestFullapiQtproxyImpl : public LogosProviderBase
{
    LOGOS_PROVIDER(TestFullapiQtproxyImpl, "test_fullapi_qtproxy", "1.0.0")

protected:
    void onInit(LogosAPI* api) override;

public:
    // ── Control ─────────────────────────────────────────────────────────────
    /// Bind the full_api interface to `moduleName` and subscribe to its events.
    LOGOS_METHOD bool useProvider(const QString& moduleName);
    /// The module name currently bound.
    LOGOS_METHOD QString currentProvider();
    /// "<eventName>:<payload-or-size>" for the most recently forwarded event.
    LOGOS_METHOD QString getLastEvent();
    /// Round-trip every array type and report the received sizes (shape only).
    LOGOS_METHOD QString probeArrays();
    /// Render the discriminating returns through the SYNC wrappers.
    LOGOS_METHOD QString syncProbe();
    /// Fire the same set through the ASYNC wrappers; returns immediately.
    LOGOS_METHOD QString probeAsync();
    /// The async renderings once the completions have landed (else "pending=N").
    LOGOS_METHOD QString getAsyncProbe();

    // ── Forwarded full_api surface ──────────────────────────────────────────
    LOGOS_METHOD QString whoAmI();
    LOGOS_METHOD QString echoString(const QString& v);
    LOGOS_METHOD QByteArray echoBytes(const QByteArray& v);
    LOGOS_METHOD qlonglong echoInt(qlonglong v);
    LOGOS_METHOD qulonglong echoUint(qulonglong v);
    LOGOS_METHOD double echoDouble(double v);
    LOGOS_METHOD bool echoBool(bool v);
    LOGOS_METHOD QVariant echoAny(const QVariant& v);
    LOGOS_METHOD QStringList echoStringList(const QStringList& v);
    LOGOS_METHOD QVariantList echoIntList(const QVariantList& v);
    LOGOS_METHOD QVariantList echoUintList(const QVariantList& v);
    LOGOS_METHOD QVariantList echoDoubleList(const QVariantList& v);
    LOGOS_METHOD QVariantList echoBoolList(const QVariantList& v);
    LOGOS_METHOD QVariantList echoList(const QVariantList& v);
    LOGOS_METHOD QVariantMap echoMap(const QVariantMap& v);
    LOGOS_METHOD void doVoid();
    LOGOS_METHOD QString echoTriple(qlonglong i, const QString& s, const QByteArray& b);
    LOGOS_METHOD LogosResult makeResult(bool ok);
    LOGOS_METHOD bool fireStringEvent(const QString& v);
    LOGOS_METHOD bool fireBytesEvent(const QByteArray& v);
    LOGOS_METHOD bool fireIntEvent(qlonglong v);
    LOGOS_METHOD bool fireUintEvent(qulonglong v);
    LOGOS_METHOD bool fireDoubleEvent(double v);
    LOGOS_METHOD bool fireBoolEvent(bool v);
    LOGOS_METHOD bool fireAnyEvent(const QVariant& v);
    LOGOS_METHOD bool fireStringListEvent(const QStringList& v);
    LOGOS_METHOD bool fireIntListEvent(const QVariantList& v);
    LOGOS_METHOD bool fireUintListEvent(const QVariantList& v);
    LOGOS_METHOD bool fireDoubleListEvent(const QVariantList& v);
    LOGOS_METHOD bool fireBoolListEvent(const QVariantList& v);
    LOGOS_METHOD bool fireListEvent(const QVariantList& v);
    LOGOS_METHOD bool fireMapEvent(const QVariantMap& v);
    LOGOS_METHOD bool fireTripleEvent(qlonglong i, const QString& s, const QByteArray& b);

private:
    // A fresh bound wrapper per call, exactly like the universal proxy's
    // `modules().bind_full_api(m_target)`. The event subscriptions register on
    // the LogosAPI-owned client, so the temporary going out of scope is fine.
    FullApi target();
    void subscribeToTarget();
    void recordAsync(const QString& key, const QString& rendered);

    LogosAPI* m_api = nullptr;
    LogosModules* m_logos = nullptr;
    QString m_provider = "test_fullapi_cpp";
    QString m_lastEvent;
    // There is no unsubscribe on the client, and re-binding must not stack
    // callbacks: subscribing twice to the same provider made every event arrive
    // twice (measured), which would double every event-position cell in the
    // matrix. Subscribe at most once per provider name, and have each callback
    // drop the delivery if its provider is no longer the bound one.
    QSet<QString> m_subscribed;

    // probeAsync() completions land on whichever thread the transport delivers
    // on; getAsyncProbe() reads them from a separate call.
    QMutex m_asyncMx;
    QVariantMap m_asyncResults;
    int m_asyncDone = 0;
};

#endif // TEST_FULLAPI_QTPROXY_IMPL_H
