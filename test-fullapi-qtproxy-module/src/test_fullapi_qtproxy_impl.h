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
//     type, in the same generated file, and nothing had ever driven the async
//     one. syncProbe() / probeAsync() + getAsyncProbe() render a FIXED input set
//     through both; `useCallMode` makes it an AXIS instead, routing every
//     forwarded call through whichever table is selected so the whole case table
//     replays twice.
//
// The forwarding surface mirrors test-fullapi-proxy-module-cpp 1:1 (33 methods,
// 15 events) so the matrix replays cases.json through this consumer unchanged:
//   logoscore call test_fullapi_qtproxy echoInt 42
//
// EVENTS have no sync/async axis: a subscription is a callback either way (the
// generated `onXxxEvent` accessors are the only form), so `useCallMode` governs
// METHODS only. An event cell measured through this proxy is the same cell in
// both modes; that is a property of the generated surface, not an omission.
//
// Every declaration below is scanned by logos-cpp-generator --provider-header,
// whose regex needs the whole declaration on ONE line ending in `;`. Do not wrap.
// ─────────────────────────────────────────────────────────────────────────────

#include "logos_provider_object.h"
#include "logos_api.h"
#include "logos_api_client.h"
#include "logos_sdk.h"
#include "logos_types.h"

// The Qt-typed surface as a VENEER over the lp path, emitted by
// `logos-qt-generator --backend consumer`. Its public surface is byte-identical
// to the generated `FullApi`'s (diffed), so `useWrapper` can swap which one
// every forwarded call goes through — the whole case table replays through both
// against the same provider in the same process.
#include "full_api_veneer_api.h"

#include <QByteArray>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include <functional>
#include <map>
#include <memory>

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
    /// Route every forwarded METHOD through the generated sync or async wrapper.
    LOGOS_METHOD bool useCallMode(const QString& mode);
    /// Route every forwarded call through the "generated" Qt wrapper or the "veneer".
    LOGOS_METHOD bool useWrapper(const QString& which);
    /// "generated" or "veneer".
    LOGOS_METHOD QString currentWrapper();
    /// Which TokenManager each path sees, and whether it holds the auth tokens.
    LOGOS_METHOD QString tokenProbe();
    /// makeResult(true) rendered through the generated wrapper and the veneer.
    LOGOS_METHOD QString resultShapeProbe();
    /// "sync" or "async".
    LOGOS_METHOD QString currentCallMode();
    /// "ok-sync" / "ok-async" (which generated table actually ran), or a failure.
    LOGOS_METHOD QString lastCallStatus();
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

    // A fresh bound veneer per call, constructed exactly like `target()` —
    // `(LogosAPI*, moduleName)`. The lp client and its RAII subscriptions live
    // in the process-lifetime LpBridge, not in this handle, so a temporary can
    // subscribe (the same contract the LogosAPI-owned client gave `FullApi`).
    FullApiVeneer veneerTarget();
    bool m_useVeneer = false;
    QSet<QString> m_veneerSubscribed;

    // ── async mode: turn a callback back into a return value ────────────────
    //
    // A Q_INVOKABLE has to answer with a value, so async mode has to WAIT. Two
    // constraints shape how:
    //
    //   * a completion is delivered on whatever thread the transport uses, and
    //     QEventLoop::quit() is not thread-safe — so the wait is a poll on a
    //     mutex-guarded slot, never a cross-thread quit();
    //   * the wait must still pump, because a same-thread completion arrives as
    //     a queued event. That nesting is not new: the SYNC path already spins a
    //     nested QEventLoop inside the qt_remote transport
    //     (remote_transport.cpp:366), so async mode adds no hazard the sync
    //     table did not already carry.
    //
    // A timeout is recorded as `lastCallStatus() == "async-timeout"` rather than
    // being papered over, because the async table substitutes a DEFAULT on a
    // missing value — 0, an empty list — which is exactly the shape of a
    // plausible-looking wrong answer. Without the status a driver could not tell
    // "the callback said 0" from "the callback never ran".
    template <typename T>
    struct AsyncSlot {
        QMutex mx;
        T value{};
        bool done = false;
    };

    /// Pump the current thread's event loop until `ready()` or the deadline.
    bool pumpUntil(const std::function<bool()>& ready);

    template <typename T, typename Start>
    T awaitAsync(Start&& start)
    {
        auto slot = std::make_shared<AsyncSlot<T>>();
        start([slot](T v) {
            QMutexLocker lk(&slot->mx);
            slot->value = v;
            slot->done = true;
        });
        const bool ok = pumpUntil([slot] { QMutexLocker lk(&slot->mx); return slot->done; });
        m_lastCallStatus = ok ? QStringLiteral("ok-async") : QStringLiteral("async-timeout");
        QMutexLocker lk(&slot->mx);
        return slot->value;
    }

    template <typename Start>
    void awaitAsyncVoid(Start&& start)
    {
        auto slot = std::make_shared<AsyncSlot<bool>>();
        start([slot] {
            QMutexLocker lk(&slot->mx);
            slot->value = true;
            slot->done = true;
        });
        const bool ok = pumpUntil([slot] { QMutexLocker lk(&slot->mx); return slot->done; });
        m_lastCallStatus = ok ? QStringLiteral("ok-async") : QStringLiteral("async-timeout");
    }

    LogosAPI* m_api = nullptr;
    LogosModules* m_logos = nullptr;
    QString m_provider = "test_fullapi_cpp";
    // sync is the default so an existing caller (and every cell measured before
    // this switch existed) keeps the surface it had.
    bool m_async = false;
    QString m_lastCallStatus = "ok-sync";
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
