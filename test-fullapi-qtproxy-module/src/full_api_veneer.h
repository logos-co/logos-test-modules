#ifndef FULL_API_VENEER_H
#define FULL_API_VENEER_H

// ─────────────────────────────────────────────────────────────────────────────
// SPIKE — the Qt-typed `full_api` wrapper as a VENEER over the lp-typed one.
//
// Same public surface as the generated Qt `FullApi` (33 sync + 33 async + 15
// typed event accessors, Qt types throughout), but every body is
// convert-args / delegate / convert-return against `FullApiLp` — the verbatim
// `--api-style lp` output checked in beside this file.
//
// THE RULE THIS FILE IS TESTING: the veneer may not hand-write a single
// conversion. Everything goes through converters that already own the mapping:
//
//   Qt      -> canonical JSON   logos::qvariantToNlohmann   (logos-protocol
//                               logos_json_convert.cpp:38)
//   JSON    -> Qt               logos::nlohmannToQVariant   (…:129)
//   JSON    -> std (LIDL type)  logos::fromJson<T>          (logos_codec.h:421)
//   std     -> JSON             logos::toJson<T>            (logos_codec.h:413)
//
// Everything a would-be generator emits is therefore a CALL naming a type, never
// a conversion. `qtlp::` below is the whole adapter: two function templates.
// Anything that could not be expressed that way is marked `LEAK n:` — those are
// the findings, and they are deliberately left visible rather than papered over.
// ─────────────────────────────────────────────────────────────────────────────

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "logos_call_error.h"
#include "logos_mode.h"           // Timeout (see LEAK 4)
#include "logos_codec.h"          // logos::toJson / logos::fromJson
#include "logos_json_convert.h"   // logos::qvariantToNlohmann / nlohmannToQVariant
#include "logos_types.h"          // LogosResult

#include "full_api_lp.h"

namespace qtlp {

// ── the entire adapter ──────────────────────────────────────────────────────

// Qt surface value -> canonical JSON. One canonical call.
inline nlohmann::json toWire(const QVariant& v)
{
    return logos::qvariantToNlohmann(v);
}

// Canonical JSON -> Qt surface value of static type QtT. One canonical call
// plus Qt's own uniform narrowing (`qvariant_cast`), which is the SAME
// narrowing the generated async table already uses — no per-type table.
template <class QtT>
QtT fromWire(const nlohmann::json& j)
{
    return qvariant_cast<QtT>(logos::nlohmannToQVariant(j));
}

// LEAK 1 — `result` has no canonical JSON -> LogosResult inverse.
//
// logos::qvariantToNlohmann DOES own LogosResult -> JSON (logos_json_convert.cpp
// :43-57). The other direction does not exist: nlohmannToQVariant turns
// {success,value,error} into a plain QVariantMap, and `qvariant_cast<LogosResult>`
// of a QVariantMap yields a default-constructed LogosResult — silently
// `success=false`. The canonical pair is ASYMMETRIC, so the veneer cannot be
// written without either (a) adding jsonToLogosResult to logos_json_convert, or
// (b) hand-writing it here — i.e. the third copy the design forbids.
//
// This specialisation is the shape (a) would take, written here only so the
// spike can RUN. It belongs in logos-protocol/cpp/logos_json_convert.cpp next to
// its inverse.
template <>
inline LogosResult fromWire<LogosResult>(const nlohmann::json& j)
{
    LogosResult r;
    r.success = false;
    if (!j.is_object()) return r;
    if (j.contains("success") && j["success"].is_boolean()) r.success = j["success"].get<bool>();
    if (j.contains("value"))  r.value = logos::nlohmannToQVariant(j["value"]);
    if (j.contains("error"))  r.error = logos::nlohmannToQVariant(j["error"]);
    return r;
}

// Qt -> the lp wrapper's std parameter type.
template <class StdT>
StdT toStd(const QVariant& v)
{
    return logos::fromJson<StdT>(toWire(v));
}

// The lp wrapper's std return type -> Qt.
template <class QtT, class StdT>
QtT fromStd(const StdT& v)
{
    return fromWire<QtT>(logos::toJson<StdT>(v));
}

// LEAK 2 — `result` has no canonical std -> JSON either.
//
// logos_codec.h's Codec<T> covers every LIDL leaf and their compositions, but
// not StdLogosResult: StdLogosResult lives in logos-cpp-sdk (logos_result.h) and
// logos_codec.h lives in logos-protocol, which cannot see it. The lp generator
// already hand-writes this encoding (generator_lib.cpp:1174-1176, `lpPushExpr`),
// and logos_lp_client.h owns only the DECODE half (`logos::jsonToStdResult`).
// So `result` is the one LIDL type whose codec is split across two repos with
// one half missing. It belongs beside jsonToStdResult in logos_lp_client.h.
template <>
inline LogosResult fromStd<LogosResult, StdLogosResult>(const StdLogosResult& v)
{
    nlohmann::json j = nlohmann::json::object();
    j["success"] = v.success;
    j["value"]   = v.value;
    j["error"]   = v.error;
    return fromWire<LogosResult>(j);
}

}  // namespace qtlp

// ─────────────────────────────────────────────────────────────────────────────
// The veneer. Surface copied verbatim from the generated Qt `FullApi` header so
// a caller cannot tell them apart at compile time (the proxy impl swaps between
// them through one generic lambda).
//
// LEAK 3 — LIFETIME. The generated Qt `FullApi` is a value returned by
// `bind_full_api()`; subscribing on a TEMPORARY works because the subscription
// lives on the LogosAPI-owned LogosAPIClient. An lp subscription is an RAII
// `LpSubscription` that unsubscribes on destruction, so a veneer that owned it
// by value would silently stop delivering when the temporary died. The lp
// generator already solved this with an umbrella-owned `State*`; the Qt veneer
// has to inherit that same indirection — which is a change to the Qt wrapper's
// OWNERSHIP contract, not just its body.
//
// LEAK 4 — `Timeout`. The Qt surface takes `Timeout timeout = Timeout()` on
// every async overload and threads it into invokeRemoteMethodAsync.
// `logos::LpClient::invokeAsync` has no timeout parameter at all (it passes
// timeout_ms = 0 → `Timeout()`), so the veneer must DROP the caller's timeout.
// Kept in the signature below and ignored, which is exactly the silent
// behaviour change a real migration would ship.
//
// LEAK 5 — the untyped event channel. `on(name, cb)` / `trigger(...)` /
// `setEventSource(...)` on the generated Qt wrapper have no lp counterpart:
// lp_* exposes typed subscribe only, and no emit-on-behalf-of at all. A veneer
// cannot cover them, so a real Qt wrapper would keep its LogosAPIClient for
// those and be a veneer only for the typed half — two clients per wrapper, and
// two transports' worth of token state.
// ─────────────────────────────────────────────────────────────────────────────

class FullApiVeneer {
public:
    // See LEAK 3: State is umbrella-owned, exactly like the lp wrapper's.
    using State = FullApiLp::State;

    explicit FullApiVeneer(State* state) : m_lp(state), m_state(state) {}

    // SPIKE PROBE — the same `result` call WITHOUT the std hop: Qt args ->
    // canonical JSON -> LpClient::invoke -> canonical JSON -> Qt. Used to show
    // that the `result` divergence the matrix found is caused by routing through
    // StdLogosResult (whose `error` is a std::string and so cannot be null), not
    // by the veneer idea. Not part of the surface.
    LogosResult makeResultNoStdHop(bool ok, logos::CallError* err = nullptr);

    // ── typed event accessors ───────────────────────────────────────────────
    bool onStringEvent(std::function<void(const QString& v)> callback);
    bool onBytesEvent(std::function<void(QByteArray v)> callback);
    bool onIntEvent(std::function<void(qlonglong v)> callback);
    bool onUintEvent(std::function<void(qulonglong v)> callback);
    bool onDoubleEvent(std::function<void(double v)> callback);
    bool onBoolEvent(std::function<void(bool v)> callback);
    bool onAnyEvent(std::function<void(QVariant v)> callback);
    bool onStringListEvent(std::function<void(const QStringList& v)> callback);
    bool onIntListEvent(std::function<void(const QVariantList& v)> callback);
    bool onUintListEvent(std::function<void(const QVariantList& v)> callback);
    bool onDoubleListEvent(std::function<void(const QVariantList& v)> callback);
    bool onBoolListEvent(std::function<void(const QVariantList& v)> callback);
    bool onListEvent(std::function<void(const QVariantList& v)> callback);
    bool onMapEvent(std::function<void(const QVariantMap& v)> callback);
    bool onTripleEvent(std::function<void(qlonglong i, const QString& s, QByteArray b)> callback);

    // ── methods ─────────────────────────────────────────────────────────────
    QString whoAmI(logos::CallError* err = nullptr);
    void whoAmIAsync(std::function<void(QString)> callback, Timeout timeout = Timeout());
    QString echoString(const QString& v, logos::CallError* err = nullptr);
    void echoStringAsync(const QString& v, std::function<void(QString)> callback, Timeout timeout = Timeout());
    QByteArray echoBytes(QByteArray v, logos::CallError* err = nullptr);
    void echoBytesAsync(QByteArray v, std::function<void(QByteArray)> callback, Timeout timeout = Timeout());
    qlonglong echoInt(qlonglong v, logos::CallError* err = nullptr);
    void echoIntAsync(qlonglong v, std::function<void(qlonglong)> callback, Timeout timeout = Timeout());
    qulonglong echoUint(qulonglong v, logos::CallError* err = nullptr);
    void echoUintAsync(qulonglong v, std::function<void(qulonglong)> callback, Timeout timeout = Timeout());
    double echoDouble(double v, logos::CallError* err = nullptr);
    void echoDoubleAsync(double v, std::function<void(double)> callback, Timeout timeout = Timeout());
    bool echoBool(bool v, logos::CallError* err = nullptr);
    void echoBoolAsync(bool v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    QVariant echoAny(QVariant v, logos::CallError* err = nullptr);
    void echoAnyAsync(QVariant v, std::function<void(QVariant)> callback, Timeout timeout = Timeout());
    QStringList echoStringList(const QStringList& v, logos::CallError* err = nullptr);
    void echoStringListAsync(const QStringList& v, std::function<void(QStringList)> callback, Timeout timeout = Timeout());
    QVariantList echoIntList(const QVariantList& v, logos::CallError* err = nullptr);
    void echoIntListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout = Timeout());
    QVariantList echoUintList(const QVariantList& v, logos::CallError* err = nullptr);
    void echoUintListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout = Timeout());
    QVariantList echoDoubleList(const QVariantList& v, logos::CallError* err = nullptr);
    void echoDoubleListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout = Timeout());
    QVariantList echoBoolList(const QVariantList& v, logos::CallError* err = nullptr);
    void echoBoolListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout = Timeout());
    QVariantList echoList(const QVariantList& v, logos::CallError* err = nullptr);
    void echoListAsync(const QVariantList& v, std::function<void(QVariantList)> callback, Timeout timeout = Timeout());
    QVariantMap echoMap(const QVariantMap& v, logos::CallError* err = nullptr);
    void echoMapAsync(const QVariantMap& v, std::function<void(QVariantMap)> callback, Timeout timeout = Timeout());
    void doVoid(logos::CallError* err = nullptr);
    void doVoidAsync(std::function<void()> callback, Timeout timeout = Timeout());
    QString echoTriple(qlonglong i, const QString& s, QByteArray b, logos::CallError* err = nullptr);
    void echoTripleAsync(qlonglong i, const QString& s, QByteArray b, std::function<void(QString)> callback, Timeout timeout = Timeout());
    LogosResult makeResult(bool ok, logos::CallError* err = nullptr);
    void makeResultAsync(bool ok, std::function<void(LogosResult)> callback, Timeout timeout = Timeout());
    bool fireStringEvent(const QString& v, logos::CallError* err = nullptr);
    void fireStringEventAsync(const QString& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireBytesEvent(QByteArray v, logos::CallError* err = nullptr);
    void fireBytesEventAsync(QByteArray v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireIntEvent(qlonglong v, logos::CallError* err = nullptr);
    void fireIntEventAsync(qlonglong v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireUintEvent(qulonglong v, logos::CallError* err = nullptr);
    void fireUintEventAsync(qulonglong v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireDoubleEvent(double v, logos::CallError* err = nullptr);
    void fireDoubleEventAsync(double v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireBoolEvent(bool v, logos::CallError* err = nullptr);
    void fireBoolEventAsync(bool v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireAnyEvent(QVariant v, logos::CallError* err = nullptr);
    void fireAnyEventAsync(QVariant v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireStringListEvent(const QStringList& v, logos::CallError* err = nullptr);
    void fireStringListEventAsync(const QStringList& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireIntListEvent(const QVariantList& v, logos::CallError* err = nullptr);
    void fireIntListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireUintListEvent(const QVariantList& v, logos::CallError* err = nullptr);
    void fireUintListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireDoubleListEvent(const QVariantList& v, logos::CallError* err = nullptr);
    void fireDoubleListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireBoolListEvent(const QVariantList& v, logos::CallError* err = nullptr);
    void fireBoolListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireListEvent(const QVariantList& v, logos::CallError* err = nullptr);
    void fireListEventAsync(const QVariantList& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireMapEvent(const QVariantMap& v, logos::CallError* err = nullptr);
    void fireMapEventAsync(const QVariantMap& v, std::function<void(bool)> callback, Timeout timeout = Timeout());
    bool fireTripleEvent(qlonglong i, const QString& s, QByteArray b, logos::CallError* err = nullptr);
    void fireTripleEventAsync(qlonglong i, const QString& s, QByteArray b, std::function<void(bool)> callback, Timeout timeout = Timeout());

private:
    FullApiLp m_lp;
    State* m_state;
};

#endif  // FULL_API_VENEER_H
