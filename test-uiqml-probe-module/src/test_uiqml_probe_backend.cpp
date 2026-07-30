#include "test_uiqml_probe_backend.h"

#include <QVariant>

#include <cstdint>
#include <exception>
#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "logos_codec.h"          // logos::fromJson<T> / logos::toJson<T>
#include "logos_json_convert.h"   // logos::qvariantToNlohmann / nlohmannToQVariant

// Generated umbrella: LogosModules (behind modules()) carrying the Qt-typed
// test_fullapi_cpp wrapper. `type: ui_qml` keeps api-style=qt, so these
// signatures are qulonglong / qlonglong / QByteArray / QVariant /
// QVariantList / QVariantMap — exactly the types the .rep's NATIVE slots
// declare, which is why a native slot can forward with no conversion at all.
#include "logos_sdk.h"

namespace {

// The probe's own reporting goes through the canonical Qt -> JSON converter, so
// the REPORT cannot be the thing that loses the value. (The obvious
// QJsonDocument::fromVariant would degrade every integer to double and flatten
// a QByteArray to a mangled string — i.e. it would manufacture exactly the
// defect under study.)
QString dump(const QVariant& v)
{
    return QString::fromStdString(logos::qvariantToNlohmann(v).dump());
}

QString report(const QVariant& sent, const QVariant& got)
{
    return QStringLiteral("sent=") + dump(sent) + QStringLiteral(" got=") + dump(got);
}

// ── the ONE decode every QString slot uses ──────────────────────────────────
//
//   text  --nlohmann parse-->      canonical JSON
//         --logos::fromJson<T>-->  the LIDL value of the DECLARED type,
//                                  or CodecError
//         --logos::toJson<T>-->    canonical JSON again
//         --logos::nlohmannToQVariant--> the Qt surface value
//
// Note what is NOT here: no per-type parsing, no sign or width handling, no
// base64. `fromJson<T>` is logos-protocol's canonical decode for the declared
// LIDL type — it is the thing that refuses -1 for a uint, 3.7 for an int and a
// string for a list — and `nlohmannToQVariant` is the canonical JSON -> Qt
// converter that already owns {"_bytes":...} -> QByteArray. Re-encoding through
// `toJson<T>` in between is deliberate rather than redundant: it means the Qt
// value handed to the wrapper is produced ONLY by a canonical converter and is
// never assembled by this file. A third copy of the conversion is precisely
// what this codebase has been deleting.
template <class T>
QVariant decodeJsonArg(const QString& text)
{
    const nlohmann::json parsed = nlohmann::json::parse(text.toStdString());
    return logos::nlohmannToQVariant(logos::toJson<T>(logos::fromJson<T>(parsed)));
}

// Shared shape for the QString slots. The decode is the only step that can
// fail, and when it does the answer says REJECTED — the whole point of the
// QString transport is that an out-of-domain value produces a visible refusal
// here instead of a silently repaired value at the provider.
template <class T, class Forward>
QString callWithJsonArg(const QString& text, Forward&& forward)
{
    QVariant arg;
    try {
        arg = decodeJsonArg<T>(text);
    } catch (const std::exception& e) {
        return QStringLiteral("REJECTED: ") + QString::fromUtf8(e.what());
    }
    return report(arg, forward(arg));
}

} // namespace

void TestUiqmlProbeBackend::onContextReady()
{
    setLastResult(QStringLiteral("READY"));
}

// ─────────────────────────────────────────────────────────────────────────────
// NATIVE transport — forward exactly what arrived.
//
// There is nothing to validate: by the time control reaches this body QML has
// already produced a value of the slot's declared C++ type, and the value the
// author actually wrote is gone. That is the declared bargain.
// ─────────────────────────────────────────────────────────────────────────────

void TestUiqmlProbeBackend::doEchoInt(qlonglong v)
{
    setLastResult(report(QVariant::fromValue(v),
                         QVariant::fromValue(modules().test_fullapi_cpp.echoInt(v))));
}

void TestUiqmlProbeBackend::doEchoUint(qulonglong v)
{
    setLastResult(report(QVariant::fromValue(v),
                         QVariant::fromValue(modules().test_fullapi_cpp.echoUint(v))));
}

void TestUiqmlProbeBackend::doEchoUintList(QVariantList v)
{
    setLastResult(report(QVariant::fromValue(v),
                         QVariant::fromValue(modules().test_fullapi_cpp.echoUintList(v))));
}

void TestUiqmlProbeBackend::doEchoMap(QVariantMap v)
{
    setLastResult(report(QVariant::fromValue(v),
                         QVariant::fromValue(modules().test_fullapi_cpp.echoMap(v))));
}

void TestUiqmlProbeBackend::doEchoAny(QVariant v)
{
    setLastResult(report(v, QVariant::fromValue(modules().test_fullapi_cpp.echoAny(v))));
}

// ─────────────────────────────────────────────────────────────────────────────
// QString transport — decode with the canonical codec, then forward the typed
// value. Same provider methods as the native twins above.
// ─────────────────────────────────────────────────────────────────────────────

void TestUiqmlProbeBackend::doEchoIntJson(QString v)
{
    setLastResult(callWithJsonArg<int64_t>(v, [this](const QVariant& a) {
        return QVariant::fromValue(
            modules().test_fullapi_cpp.echoInt(qvariant_cast<qlonglong>(a)));
    }));
}

void TestUiqmlProbeBackend::doEchoUintJson(QString v)
{
    setLastResult(callWithJsonArg<uint64_t>(v, [this](const QVariant& a) {
        return QVariant::fromValue(
            modules().test_fullapi_cpp.echoUint(qvariant_cast<qulonglong>(a)));
    }));
}

void TestUiqmlProbeBackend::doEchoBytesJson(QString v)
{
    setLastResult(callWithJsonArg<std::vector<uint8_t>>(v, [this](const QVariant& a) {
        return QVariant::fromValue(
            modules().test_fullapi_cpp.echoBytes(a.toByteArray()));
    }));
}

void TestUiqmlProbeBackend::doEchoUintListJson(QString v)
{
    setLastResult(callWithJsonArg<std::vector<uint64_t>>(v, [this](const QVariant& a) {
        return QVariant::fromValue(
            modules().test_fullapi_cpp.echoUintList(a.toList()));
    }));
}

void TestUiqmlProbeBackend::doEchoMapJson(QString v)
{
    setLastResult(callWithJsonArg<std::map<std::string, nlohmann::json>>(
        v, [this](const QVariant& a) {
            return QVariant::fromValue(
                modules().test_fullapi_cpp.echoMap(a.toMap()));
        }));
}

void TestUiqmlProbeBackend::doEchoAnyJson(QString v)
{
    setLastResult(callWithJsonArg<nlohmann::json>(v, [this](const QVariant& a) {
        return QVariant::fromValue(modules().test_fullapi_cpp.echoAny(a));
    }));
}

// ─────────────────────────────────────────────────────────────────────────────
// Carried over from the original probe (native only), so the nine hostile cases
// it first measured still run in this binary.
// ─────────────────────────────────────────────────────────────────────────────

void TestUiqmlProbeBackend::doEchoBool(bool v)
{
    setLastResult(report(QVariant::fromValue(v),
                         QVariant::fromValue(modules().test_fullapi_cpp.echoBool(v))));
}

void TestUiqmlProbeBackend::doEchoStringList(QStringList v)
{
    setLastResult(report(QVariant::fromValue(v),
                         QVariant::fromValue(modules().test_fullapi_cpp.echoStringList(v))));
}

void TestUiqmlProbeBackend::doEchoIntList(QVariantList v)
{
    setLastResult(report(QVariant::fromValue(v),
                         QVariant::fromValue(modules().test_fullapi_cpp.echoIntList(v))));
}

void TestUiqmlProbeBackend::doEchoList(QVariantList v)
{
    setLastResult(report(QVariant::fromValue(v),
                         QVariant::fromValue(modules().test_fullapi_cpp.echoList(v))));
}
