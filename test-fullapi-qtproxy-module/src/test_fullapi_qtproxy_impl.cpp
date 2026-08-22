#include "test_fullapi_qtproxy_impl.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include <functional>
#include <memory>

#include "logos_protocol.h"    // lp_token_get / lp_token_get_for / lp_string_free
#include "logos_qt_wire.h"     // logos::qt::toWire / fromWire<T> — the canonical edge

// Generated at build time. metadata.json declares `interface_dependencies`, so
// this umbrella carries `FullApi bind_full_api(const QString&)`; it also sets
// `codegen.consumer_api_style: "qt"`, so `FullApi` is the QT-TYPED wrapper and
// the umbrella is the origin-bound one — default-constructible, holding no
// LogosAPI, stating this module's own name as the call origin.
#include "logos_sdk.h"

// ─── The two sides of this module ────────────────────────────────────────────
//
// The header is the PROVIDER surface and is std-typed, because the LIDL contract
// is derived from it and the C exports are `logos_module_impl.h`'s. Everything
// below the line in this file is the CONSUMER surface and is Qt-typed, because
// that is the path being measured. So every forwarded method converts once on
// the way in and once on the way out, and the conversion vocabulary is the
// canonical one (`logos::qt::toWire` / `fromWire<T>`, which sit on
// logos-protocol's single deduped codec) — never a hand-rolled table, which is
// the defect class this whole fixture exists to catch.

// ─── Rendering ───────────────────────────────────────────────────────────────
//
// syncProbe() and getAsyncProbe() must be comparable to each other and to what
// the provider actually holds, so the rendering has to be LOSSLESS for the types
// the matrix cares about. QJsonDocument is not an option: it degrades a uint64
// above int64max to a double, which is precisely the value that discriminates
// the sync return path from the async one. Hence a hand-rolled, type-tagged
// rendering, unchanged from the pre-migration fixture so the strings a driver
// recorded before still compare.
//
//   s:<text>  tstr        i:<n>   int         u:<n>   uint
//   d:<n>     float64     B:<t|f> bool        b:<hex> bstr
//   [..]      list        {k=v}   map         R(..)   result
//   -         invalid / absent

namespace {

QString renderVariant(const QVariant& v);

// The Qt consumer hands a LIDL `[int]` back as QList<qlonglong>, not
// QVariantList — a typed container QVariant has no implicit constructor for, and
// which renderVariant()'s userType() switch would therefore never reach. Render
// the ELEMENTS through renderVariant() instead, which keeps the string
// byte-identical to the QVariantList rendering that preceded the widening
// (`[i:1,i:2]`, `[B:t,B:f]`, …) so a recorded sync/async diff still compares.
template <typename T>
QString renderTypedList(const QList<T>& l)
{
    QStringList parts;
    for (const T& e : l) parts << renderVariant(QVariant::fromValue(e));
    return "[" + parts.join(",") + "]";
}

QString renderList(const QVariantList& l)
{
    QStringList parts;
    for (const QVariant& e : l) parts << renderVariant(e);
    return "[" + parts.join(",") + "]";
}

QString renderMap(const QVariantMap& m)
{
    QStringList parts;
    for (auto it = m.constBegin(); it != m.constEnd(); ++it)
        parts << it.key() + "=" + renderVariant(it.value());
    return "{" + parts.join(",") + "}";
}

QString renderVariant(const QVariant& v)
{
    if (!v.isValid()) return "-";

    const int logosResultId = QMetaType::fromName("LogosResult").id();
    if (logosResultId != QMetaType::UnknownType && v.userType() == logosResultId) {
        const LogosResult r = v.value<LogosResult>();
        return "R(ok=" + QString(r.success ? "t" : "f")
             + ",v=" + renderVariant(r.value)
             + ",e=" + renderVariant(r.error) + ")";
    }

    // The typed containers, before the switch: their metatype ids are not
    // constant expressions, so they cannot be `case` labels. Without these a
    // QVariant carrying one would render as `?QList<qlonglong>:` — a silent hole
    // in a fixture whose whole job is to make decoding differences visible.
    if (v.userType() == qMetaTypeId<QList<qlonglong>>())  return renderTypedList(v.value<QList<qlonglong>>());
    if (v.userType() == qMetaTypeId<QList<qulonglong>>()) return renderTypedList(v.value<QList<qulonglong>>());
    if (v.userType() == qMetaTypeId<QList<double>>())     return renderTypedList(v.value<QList<double>>());
    if (v.userType() == qMetaTypeId<QList<bool>>())       return renderTypedList(v.value<QList<bool>>());

    switch (v.userType()) {
    case QMetaType::QByteArray:  return "b:" + QString::fromLatin1(v.toByteArray().toHex());
    case QMetaType::QString:     return "s:" + v.toString();
    case QMetaType::Bool:        return QString("B:") + (v.toBool() ? "t" : "f");
    case QMetaType::Int:         return "i:" + QString::number(v.toInt());
    case QMetaType::LongLong:    return "i:" + QString::number(v.toLongLong());
    case QMetaType::UInt:        return "u:" + QString::number(v.toUInt());
    case QMetaType::ULongLong:   return "u:" + QString::number(v.toULongLong());
    case QMetaType::Double:      return "d:" + QString::number(v.toDouble(), 'g', 17);
    case QMetaType::QStringList: {
        QStringList parts;
        for (const QString& s : v.toStringList()) parts << "s:" + s;
        return "[" + parts.join(",") + "]";
    }
    case QMetaType::QVariantList: return renderList(v.toList());
    case QMetaType::QVariantMap:  return renderMap(v.toMap());
    default: break;
    }
    return "?" + QString::fromLatin1(v.typeName()) + ":" + v.toString();
}

QString jsonEscape(const QString& s)
{
    QString out;
    out.reserve(s.size() + 8);
    for (const QChar c : s) {
        if (c == '"' || c == '\\') out += '\\';
        if (c == '\n') { out += "\\n"; continue; }
        out += c;
    }
    return out;
}

QString renderTable(const std::map<std::string, QString>& results)
{
    QStringList parts;
    for (const auto& kv : results)
        parts << "\"" + jsonEscape(QString::fromStdString(kv.first)) + "\":\""
                 + jsonEscape(kv.second) + "\"";
    return "{" + parts.join(",") + "}";
}

// ─── std <-> Qt, at the provider/consumer boundary ───────────────────────────
//
// Scalars are casts. Everything with structure goes through logos::qt::toWire /
// fromWire<T>, i.e. the canonical codec — deliberately, so this module cannot
// become a fourth private conversion table sitting between the two it is meant
// to compare.

QString qs(const std::string& s) { return QString::fromStdString(s); }
std::string ss(const QString& s) { return s.toStdString(); }

QByteArray qb(const std::vector<uint8_t>& v)
{
    if (v.empty()) return QByteArray();
    return QByteArray(reinterpret_cast<const char*>(v.data()), qsizetype(v.size()));
}
std::vector<uint8_t> sb(const QByteArray& b)
{
    const auto* p = reinterpret_cast<const uint8_t*>(b.constData());
    return std::vector<uint8_t>(p, p + b.size());
}

QStringList qsl(const std::vector<std::string>& v)
{
    QStringList o;
    o.reserve(int(v.size()));
    for (const auto& s : v) o << qs(s);
    return o;
}
std::vector<std::string> ssl(const QStringList& v)
{
    std::vector<std::string> o;
    o.reserve(size_t(v.size()));
    for (const auto& s : v) o.push_back(ss(s));
    return o;
}

// `[int]` / `[uint]` / `[float64]` / `[bool]` are QList<qlonglong> /
// QList<qulonglong> / QList<double> / QList<bool> on the Qt consumer surface.
// They used to be QVariantList, which meant every element was boxed and the
// element type was only a convention; it is now in the type, so these
// converters lost their per-element QVariant round-trip rather than gaining
// one. `[any]` ({tstr:any}) stays QVariantList (QVariantMap) — `any` is the one
// LIDL type with no narrower Qt spelling.
QList<qlonglong> qIntList(const std::vector<int64_t>& v)
{
    QList<qlonglong> o;
    o.reserve(qsizetype(v.size()));
    for (auto e : v) o << qlonglong(e);
    return o;
}
QList<qulonglong> qUintList(const std::vector<uint64_t>& v)
{
    QList<qulonglong> o;
    o.reserve(qsizetype(v.size()));
    for (auto e : v) o << qulonglong(e);
    return o;
}
QList<double> qDoubleList(const std::vector<double>& v)
{
    QList<double> o;
    o.reserve(qsizetype(v.size()));
    for (auto e : v) o << e;
    return o;
}
QList<bool> qBoolList(const std::vector<bool>& v)
{
    QList<bool> o;
    o.reserve(qsizetype(v.size()));
    for (bool e : v) o << e;
    return o;
}

std::vector<int64_t> sIntList(const QList<qlonglong>& v)
{
    std::vector<int64_t> o;
    o.reserve(size_t(v.size()));
    for (qlonglong e : v) o.push_back(int64_t(e));
    return o;
}
std::vector<uint64_t> sUintList(const QList<qulonglong>& v)
{
    std::vector<uint64_t> o;
    o.reserve(size_t(v.size()));
    for (qulonglong e : v) o.push_back(uint64_t(e));
    return o;
}
std::vector<double> sDoubleList(const QList<double>& v)
{
    std::vector<double> o;
    o.reserve(size_t(v.size()));
    for (double e : v) o.push_back(e);
    return o;
}
std::vector<bool> sBoolList(const QList<bool>& v)
{
    std::vector<bool> o;
    o.reserve(size_t(v.size()));
    for (bool e : v) o.push_back(e);
    return o;
}

QVariant qAny(const nlohmann::json& j)   { return logos::qt::fromWire<QVariant>(j); }
nlohmann::json sAny(const QVariant& v)   { return logos::qt::toWire(v); }
QVariantList qList(const LogosList& j)   { return logos::qt::fromWire<QVariantList>(j); }
LogosList sList(const QVariantList& v)   { return logos::qt::toWire(QVariant::fromValue(v)); }
QVariantMap qMap(const LogosMap& j)      { return logos::qt::fromWire<QVariantMap>(j); }
LogosMap sMap(const QVariantMap& v)      { return logos::qt::toWire(QVariant::fromValue(v)); }

// The one asymmetric conversion, and the reason resultShapeProbe still exists.
// LogosResult::error is a QVariant, so ABSENT and EMPTY are distinguishable;
// StdLogosResult::error is a std::string, so they are not. Recorded rather than
// smoothed over: this is the boundary the migration MOVED, from "generated Qt
// wrapper vs veneer" to "Qt consumer wrapper vs this module's own std surface".
int64_t  sInt(qlonglong v)   { return int64_t(v); }
uint64_t sUint(qulonglong v) { return uint64_t(v); }
double   sDouble(double v)   { return v; }
bool     sBool(bool v)       { return v; }

StdLogosResult sResult(const LogosResult& r)
{
    StdLogosResult o;
    o.success = r.success;
    o.value = logos::qt::toWire(r.value);
    o.error = r.error.isValid() ? r.error.toString().toStdString() : std::string();
    return o;
}

// ── async mode: turn a callback back into a return value ────────────────────
//
// A dispatched method has to answer with a value, so async mode has to WAIT.
// Two constraints shape how:
//
//   * a completion is delivered on whatever thread the transport uses, and
//     QEventLoop::quit() is not thread-safe — so the wait is a poll on a
//     mutex-guarded slot, never a cross-thread quit();
//   * the wait must still pump, because a same-thread completion arrives as a
//     queued event.
//
// A timeout is recorded as `lastCallStatus() == "async-timeout"` rather than
// being papered over, because the async path substitutes a DEFAULT on a missing
// value — 0, an empty list — which is exactly the shape of a plausible-looking
// wrong answer. Without the status a driver could not tell "the callback said 0"
// from "the callback never ran".

// Pump the current thread's event loop until `ready()` or the deadline.
bool pumpUntil(const std::function<bool()>& ready)
{
    // 25s: longer than the driver's per-call timeout, so a hung completion is
    // reported by the DRIVER as the timeout it is instead of being converted
    // here into a default value that looks like an answer.
    constexpr qint64 kDeadlineMs = 25000;
    QElapsedTimer clock;
    clock.start();
    while (!ready()) {
        if (clock.elapsed() > kDeadlineMs) return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        if (ready()) break;
        QThread::msleep(1);
    }
    return true;
}

template <typename T, typename Start>
bool awaitAsyncValue(Start&& start, T& out)
{
    struct Slot {
        std::mutex mx;
        T value{};
        bool done = false;
    };
    auto slot = std::make_shared<Slot>();
    start([slot](T v) {
        std::lock_guard<std::mutex> lk(slot->mx);
        slot->value = v;
        slot->done = true;
    });
    const bool ok = pumpUntil([slot] {
        std::lock_guard<std::mutex> lk(slot->mx);
        return slot->done;
    });
    std::lock_guard<std::mutex> lk(slot->mx);
    out = slot->value;
    return ok;
}

template <typename Start>
bool awaitAsyncVoid(Start&& start)
{
    struct Slot {
        std::mutex mx;
        bool done = false;
    };
    auto slot = std::make_shared<Slot>();
    start([slot] {
        std::lock_guard<std::mutex> lk(slot->mx);
        slot->done = true;
    });
    return pumpUntil([slot] {
        std::lock_guard<std::mutex> lk(slot->mx);
        return slot->done;
    });
}

// A fresh bound wrapper per call, exactly like the universal proxy's
// `modules().bind_full_api(m_target)`. The lp client and its RAII subscriptions
// live in the process-lifetime LpBridge, not in this handle, so a temporary can
// subscribe and a temporary can outlive nothing that matters.
FullApi bindTo(const LogosModuleContext& self, const std::string& provider)
{
    // `modules()` is a const member returning a non-const reference, so a const
    // impl can still bind — no cast needed.
    return self.modules().bind_full_api(qs(provider));
}

// The shared probe inputs. Chosen so a value that survives one path and not the
// other is visible: uint64 max has no signed representation, the int is outside
// double's exact range, and the bytes carry 0x00 / 0x80 / 0xFF.
const qlonglong  kProbeInt  = Q_INT64_C(-9007199254740993);
const qulonglong kProbeUint = Q_UINT64_C(18446744073709551615);

QByteArray probeBytes()
{
    QByteArray b;
    b.append(char(0x00));
    b.append(char(0x80));
    b.append(char(0xFF));
    return b;
}

const int kAsyncExpected = 18;

} // namespace

#define TARGET bindTo(*this, m_provider)

// ─── Lifecycle + control ─────────────────────────────────────────────────────

void TestFullapiQtproxyImpl::onContextReady()
{
    qDebug() << "TestFullapiQtproxyImpl: initialized (universal provider, qt consumer)";
    subscribeToTarget();
}

bool TestFullapiQtproxyImpl::useProvider(const std::string& moduleName)
{
    m_provider = moduleName;
    subscribeToTarget();
    return true;
}

bool TestFullapiQtproxyImpl::useCallMode(const std::string& mode)
{
    // Reject anything else rather than defaulting: a typo that silently left the
    // proxy in sync mode would make the async half of the matrix a duplicate of
    // the sync half — 86 green cells proving nothing.
    if (mode != "sync" && mode != "async") return false;
    m_async = (mode == "async");
    m_lastCallStatus = "ok";
    return true;
}

std::string TestFullapiQtproxyImpl::currentCallMode() { return m_async ? "async" : "sync"; }
std::string TestFullapiQtproxyImpl::lastCallStatus()  { return m_lastCallStatus; }
std::string TestFullapiQtproxyImpl::currentProvider() { return m_provider; }
std::string TestFullapiQtproxyImpl::getLastEvent()    { return m_lastEvent; }

// Is this IMAGE's token store the one the outbound lp client reads, and does it
// hold anything?
//
// Before the migration this compared the LogosAPI's TokenManager with
// `TokenManager::instance()` — the plugin/host mirroring problem
// `logos::qt::LpBridge::syncTokens` exists to solve. A cdylib has no LogosAPI
// and no mirroring: `logos_module_accept_token` writes straight into the store
// `lp_client_create` reads, which is exactly why `consumer_api_style: "qt"` is
// safe here and refused for a Qt plugin. So the probe now reports the store
// itself, including whether this origin was given a private one.
std::string TestFullapiQtproxyImpl::tokenProbe()
{
    auto take = [](char* s) {
        const std::string out = s ? std::string(s) : std::string();
        if (s) lp_string_free(s);
        return out;
    };
    const std::string self = "test_fullapi_qtproxy";
    const bool isolated = lp_token_identity_is_isolated(self.c_str()) == 1;
    const std::string cap = isolated ? take(lp_token_get_for(self.c_str(), "capability_module"))
                                     : take(lp_token_get("capability_module"));
    const std::string prov = isolated ? take(lp_token_get_for(self.c_str(), m_provider.c_str()))
                                      : take(lp_token_get(m_provider.c_str()));
    return "origin=" + self
         + " isolated=" + (isolated ? "yes" : "no")
         + " cap=" + (cap.empty() ? "no" : "yes")
         + " prov=" + (prov.empty() ? "no" : "yes");
}

std::string TestFullapiQtproxyImpl::probeArrays()
{
    FullApi p = TARGET;
    const QList<qlonglong>  il = p.echoIntList(QList<qlonglong>{1, 2, 3});
    const QList<qulonglong> ul = p.echoUintList(QList<qulonglong>{1, 2});
    const QList<double>     dl = p.echoDoubleList(QList<double>{1.5, 2.5});
    const QList<bool>       bl = p.echoBoolList(QList<bool>{true, false});
    const QStringList  sl = p.echoStringList(QStringList{"a", "b"});
    const QVariantList al = p.echoList(QVariantList{1, 2, 3});
    return ss(QString("intList=%1 uintList=%2 doubleList=%3 boolList=%4 stringList=%5 anyList=%6")
        .arg(il.size()).arg(ul.size()).arg(dl.size())
        .arg(bl.size()).arg(sl.size()).arg(al.size()));
}

// ─── sync vs async, same inputs ──────────────────────────────────────────────
//
// syncProbe() and getAsyncProbe() render the same 18 calls through the sync and
// the async wrapper in the same format, so a driver diffs two strings.

std::string TestFullapiQtproxyImpl::syncProbe()
{
    FullApi p = TARGET;
    std::map<std::string, QString> r;
    r["whoAmI"]         = renderVariant(p.whoAmI());
    r["echoString"]     = renderVariant(p.echoString("zz"));
    r["echoInt"]        = renderVariant(QVariant::fromValue(p.echoInt(kProbeInt)));
    r["echoUint"]       = renderVariant(QVariant::fromValue(p.echoUint(kProbeUint)));
    r["echoDouble"]     = renderVariant(p.echoDouble(2.5));
    r["echoBool"]       = renderVariant(p.echoBool(true));
    r["echoBytes"]      = renderVariant(p.echoBytes(probeBytes()));
    r["echoAny"]        = renderVariant(p.echoAny(QVariant(QString("x"))));
    r["echoStringList"] = renderVariant(p.echoStringList(QStringList{"a", "b"}));
    r["echoIntList"]    = renderTypedList(p.echoIntList(QList<qlonglong>{kProbeInt}));
    r["echoUintList"]   = renderTypedList(p.echoUintList(QList<qulonglong>{kProbeUint}));
    r["echoDoubleList"] = renderTypedList(p.echoDoubleList(QList<double>{1.5, 2.5}));
    r["echoBoolList"]   = renderTypedList(p.echoBoolList(QList<bool>{true, false}));
    r["echoList"]       = renderVariant(p.echoList(QVariantList{1, QString("a"), true}));
    r["echoMap"]        = renderVariant(p.echoMap(QVariantMap{{"k", "v"}}));
    r["echoTriple"]     = renderVariant(p.echoTriple(7, "s", probeBytes()));
    r["makeResult"]     = renderVariant(QVariant::fromValue(p.makeResult(true)));
    p.doVoid();
    r["doVoid"]         = "void:ok";
    return ss(renderTable(r));
}

// `result` is the one type whose two renderings can legitimately differ, and
// after the migration the pair worth comparing changed. It used to be
// "generated Qt wrapper vs hand-committed veneer" — an axis that no longer
// exists, because the generated Qt wrapper IS the veneer. It is now the
// consumer/provider boundary INSIDE this module: what the Qt wrapper decoded,
// and what survives conversion to the std-typed surface this module re-exposes.
// StdLogosResult::error is a std::string, so an ABSENT error becomes an empty
// one; that loss is real, is this module's own, and is visible here.
std::string TestFullapiQtproxyImpl::resultShapeProbe()
{
    const LogosResult qtSide = TARGET.makeResult(true);
    const StdLogosResult stdSide = sResult(qtSide);
    // `-` is reserved for ABSENT and must not be reused for an EMPTY std string,
    // or the probe hides the very collapse it exists to show: an unset
    // LogosResult::error renders `e=-`, and the std side it becomes renders
    // `e=s:` — an empty tstr that is present. Rendering both as `-` made the two
    // sides agree by construction (measured: they did, on the first run).
    return ss("qt=" + renderVariant(QVariant::fromValue(qtSide))
            + " std=R(ok=" + QString(stdSide.success ? "t" : "f")
            + ",v=" + renderVariant(logos::qt::fromWire<QVariant>(stdSide.value))
            + ",e=s:" + qs(stdSide.error) + ")");
}

void TestFullapiQtproxyImpl::recordAsync(const std::string& key, const std::string& rendered)
{
    std::lock_guard<std::mutex> lk(m_asyncMx);
    m_asyncResults[key] = rendered;
    ++m_asyncDone;
}

std::string TestFullapiQtproxyImpl::probeAsync()
{
    {
        std::lock_guard<std::mutex> lk(m_asyncMx);
        m_asyncResults.clear();
        m_asyncDone = 0;
    }
    FullApi p = TARGET;
    p.whoAmIAsync([this](QString v) { recordAsync("whoAmI", ss(renderVariant(v))); });
    p.echoStringAsync("zz", [this](QString v) { recordAsync("echoString", ss(renderVariant(v))); });
    p.echoIntAsync(kProbeInt, [this](qlonglong v) { recordAsync("echoInt", ss(renderVariant(QVariant::fromValue(v)))); });
    p.echoUintAsync(kProbeUint, [this](qulonglong v) { recordAsync("echoUint", ss(renderVariant(QVariant::fromValue(v)))); });
    p.echoDoubleAsync(2.5, [this](double v) { recordAsync("echoDouble", ss(renderVariant(v))); });
    p.echoBoolAsync(true, [this](bool v) { recordAsync("echoBool", ss(renderVariant(v))); });
    p.echoBytesAsync(probeBytes(), [this](QByteArray v) { recordAsync("echoBytes", ss(renderVariant(v))); });
    p.echoAnyAsync(QVariant(QString("x")), [this](QVariant v) { recordAsync("echoAny", ss(renderVariant(v))); });
    p.echoStringListAsync(QStringList{"a", "b"}, [this](QStringList v) { recordAsync("echoStringList", ss(renderVariant(v))); });
    p.echoIntListAsync(QList<qlonglong>{kProbeInt}, [this](QList<qlonglong> v) { recordAsync("echoIntList", ss(renderTypedList(v))); });
    p.echoUintListAsync(QList<qulonglong>{kProbeUint}, [this](QList<qulonglong> v) { recordAsync("echoUintList", ss(renderTypedList(v))); });
    p.echoDoubleListAsync(QList<double>{1.5, 2.5}, [this](QList<double> v) { recordAsync("echoDoubleList", ss(renderTypedList(v))); });
    p.echoBoolListAsync(QList<bool>{true, false}, [this](QList<bool> v) { recordAsync("echoBoolList", ss(renderTypedList(v))); });
    p.echoListAsync(QVariantList{1, QString("a"), true}, [this](QVariantList v) { recordAsync("echoList", ss(renderVariant(v))); });
    p.echoMapAsync(QVariantMap{{"k", "v"}}, [this](QVariantMap v) { recordAsync("echoMap", ss(renderVariant(v))); });
    p.echoTripleAsync(7, "s", probeBytes(), [this](QString v) { recordAsync("echoTriple", ss(renderVariant(v))); });
    p.makeResultAsync(true, [this](LogosResult v) { recordAsync("makeResult", ss(renderVariant(QVariant::fromValue(v)))); });
    p.doVoidAsync([this]() { recordAsync("doVoid", "void:ok"); });
    return "started";
}

std::string TestFullapiQtproxyImpl::getAsyncProbe()
{
    std::lock_guard<std::mutex> lk(m_asyncMx);
    if (m_asyncDone < kAsyncExpected)
        return "pending=" + std::to_string(m_asyncDone);
    std::map<std::string, QString> rendered;
    for (const auto& kv : m_asyncResults) rendered[kv.first] = qs(kv.second);
    return ss(renderTable(rendered));
}

// ─── Forwarded methods ───────────────────────────────────────────────────────
//
// Every one of the 33 goes through BOTH the sync and the async wrapper,
// selected by useCallMode. Routing the whole case table through both is the only
// way that difference gets measured rather than reasoned about.
//
// FWD/FWD_VOID keep the pair adjacent so a method cannot be forwarded in one
// mode and forgotten in the other — the failure mode this whole module exists to
// remove, one level down.
//
// `TO_STD` is the boundary conversion for the RETURN. It is a named function,
// never an inline expression, so the whole set of conversions this module
// performs is one readable block above rather than 33 scattered casts.

#define FWD(QT_T, TO_STD, CALL, ASYNC_CALL)                                     \
    do {                                                                        \
        FullApi _p = TARGET;                                                    \
        if (!m_async) { m_lastCallStatus = "ok-sync"; return TO_STD(_p.CALL); } \
        QT_T _v{};                                                              \
        const bool _ok = awaitAsyncValue<QT_T>(                                 \
            [&](std::function<void(QT_T)> cb) { _p.ASYNC_CALL; }, _v);          \
        m_lastCallStatus = _ok ? "ok-async" : "async-timeout";                  \
        return TO_STD(_v);                                                      \
    } while (0)

std::string TestFullapiQtproxyImpl::whoAmI()
{
    FWD(QString, ss, whoAmI(), whoAmIAsync(cb));
}
std::string TestFullapiQtproxyImpl::echoString(const std::string& v)
{
    const QString qv = qs(v);
    FWD(QString, ss, echoString(qv), echoStringAsync(qv, cb));
}
std::vector<uint8_t> TestFullapiQtproxyImpl::echoBytes(const std::vector<uint8_t>& v)
{
    const QByteArray qv = qb(v);
    FWD(QByteArray, sb, echoBytes(qv), echoBytesAsync(qv, cb));
}
int64_t TestFullapiQtproxyImpl::echoInt(int64_t v)
{
    const qlonglong qv = qlonglong(v);
    FWD(qlonglong, sInt, echoInt(qv), echoIntAsync(qv, cb));
}
uint64_t TestFullapiQtproxyImpl::echoUint(uint64_t v)
{
    const qulonglong qv = qulonglong(v);
    FWD(qulonglong, sUint, echoUint(qv), echoUintAsync(qv, cb));
}
double TestFullapiQtproxyImpl::echoDouble(double v)
{
    FWD(double, sDouble, echoDouble(v), echoDoubleAsync(v, cb));
}
bool TestFullapiQtproxyImpl::echoBool(bool v)
{
    FWD(bool, sBool, echoBool(v), echoBoolAsync(v, cb));
}
nlohmann::json TestFullapiQtproxyImpl::echoAny(const nlohmann::json& v)
{
    const QVariant qv = qAny(v);
    FWD(QVariant, sAny, echoAny(qv), echoAnyAsync(qv, cb));
}
std::vector<std::string> TestFullapiQtproxyImpl::echoStringList(const std::vector<std::string>& v)
{
    const QStringList qv = qsl(v);
    FWD(QStringList, ssl, echoStringList(qv), echoStringListAsync(qv, cb));
}
std::vector<int64_t> TestFullapiQtproxyImpl::echoIntList(const std::vector<int64_t>& v)
{
    const QList<qlonglong> qv = qIntList(v);
    FWD(QList<qlonglong>, sIntList, echoIntList(qv), echoIntListAsync(qv, cb));
}
std::vector<uint64_t> TestFullapiQtproxyImpl::echoUintList(const std::vector<uint64_t>& v)
{
    const QList<qulonglong> qv = qUintList(v);
    FWD(QList<qulonglong>, sUintList, echoUintList(qv), echoUintListAsync(qv, cb));
}
std::vector<double> TestFullapiQtproxyImpl::echoDoubleList(const std::vector<double>& v)
{
    const QList<double> qv = qDoubleList(v);
    FWD(QList<double>, sDoubleList, echoDoubleList(qv), echoDoubleListAsync(qv, cb));
}
std::vector<bool> TestFullapiQtproxyImpl::echoBoolList(const std::vector<bool>& v)
{
    const QList<bool> qv = qBoolList(v);
    FWD(QList<bool>, sBoolList, echoBoolList(qv), echoBoolListAsync(qv, cb));
}
LogosList TestFullapiQtproxyImpl::echoList(const LogosList& v)
{
    const QVariantList qv = qList(v);
    FWD(QVariantList, sList, echoList(qv), echoListAsync(qv, cb));
}
LogosMap TestFullapiQtproxyImpl::echoMap(const LogosMap& v)
{
    const QVariantMap qv = qMap(v);
    FWD(QVariantMap, sMap, echoMap(qv), echoMapAsync(qv, cb));
}
std::string TestFullapiQtproxyImpl::echoTriple(int64_t i, const std::string& s, const std::vector<uint8_t>& b)
{
    const qlonglong qi = qlonglong(i);
    const QString qsv = qs(s);
    const QByteArray qbv = qb(b);
    FWD(QString, ss, echoTriple(qi, qsv, qbv), echoTripleAsync(qi, qsv, qbv, cb));
}
StdLogosResult TestFullapiQtproxyImpl::makeResult(bool ok)
{
    FWD(LogosResult, sResult, makeResult(ok), makeResultAsync(ok, cb));
}

// The one method whose async callback takes no argument, so it cannot go through
// awaitAsyncValue<T>. Split out rather than faked with a dummy T: `void` is the
// LIDL type whose two backends already told the same lie once (registry M2), and
// a dummy return here would be a third place to hide it.
void TestFullapiQtproxyImpl::doVoid()
{
    FullApi p = TARGET;
    if (!m_async) { m_lastCallStatus = "ok-sync"; p.doVoid(); return; }
    const bool ok = awaitAsyncVoid([&](std::function<void()> cb) { p.doVoidAsync(cb); });
    m_lastCallStatus = ok ? "ok-async" : "async-timeout";
}

// ─── Forwarded event triggers ────────────────────────────────────────────────
//
// These are ordinary `-> bool` methods, so they take the call-mode axis too. The
// EVENT they cause is delivered through the subscription callbacks below, which
// have no sync/async variants — see the header.

bool TestFullapiQtproxyImpl::fireStringEvent(const std::string& v)
{
    const QString qv = qs(v);
    FWD(bool, sBool, fireStringEvent(qv), fireStringEventAsync(qv, cb));
}
bool TestFullapiQtproxyImpl::fireBytesEvent(const std::vector<uint8_t>& v)
{
    const QByteArray qv = qb(v);
    FWD(bool, sBool, fireBytesEvent(qv), fireBytesEventAsync(qv, cb));
}
bool TestFullapiQtproxyImpl::fireIntEvent(int64_t v)
{
    const qlonglong qv = qlonglong(v);
    FWD(bool, sBool, fireIntEvent(qv), fireIntEventAsync(qv, cb));
}
bool TestFullapiQtproxyImpl::fireUintEvent(uint64_t v)
{
    const qulonglong qv = qulonglong(v);
    FWD(bool, sBool, fireUintEvent(qv), fireUintEventAsync(qv, cb));
}
bool TestFullapiQtproxyImpl::fireDoubleEvent(double v)
{
    FWD(bool, sBool, fireDoubleEvent(v), fireDoubleEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireBoolEvent(bool v)
{
    FWD(bool, sBool, fireBoolEvent(v), fireBoolEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireAnyEvent(const nlohmann::json& v)
{
    const QVariant qv = qAny(v);
    FWD(bool, sBool, fireAnyEvent(qv), fireAnyEventAsync(qv, cb));
}
bool TestFullapiQtproxyImpl::fireStringListEvent(const std::vector<std::string>& v)
{
    const QStringList qv = qsl(v);
    FWD(bool, sBool, fireStringListEvent(qv), fireStringListEventAsync(qv, cb));
}
bool TestFullapiQtproxyImpl::fireIntListEvent(const std::vector<int64_t>& v)
{
    const QList<qlonglong> qv = qIntList(v);
    FWD(bool, sBool, fireIntListEvent(qv), fireIntListEventAsync(qv, cb));
}
bool TestFullapiQtproxyImpl::fireUintListEvent(const std::vector<uint64_t>& v)
{
    const QList<qulonglong> qv = qUintList(v);
    FWD(bool, sBool, fireUintListEvent(qv), fireUintListEventAsync(qv, cb));
}
bool TestFullapiQtproxyImpl::fireDoubleListEvent(const std::vector<double>& v)
{
    const QList<double> qv = qDoubleList(v);
    FWD(bool, sBool, fireDoubleListEvent(qv), fireDoubleListEventAsync(qv, cb));
}
bool TestFullapiQtproxyImpl::fireBoolListEvent(const std::vector<bool>& v)
{
    const QList<bool> qv = qBoolList(v);
    FWD(bool, sBool, fireBoolListEvent(qv), fireBoolListEventAsync(qv, cb));
}
bool TestFullapiQtproxyImpl::fireListEvent(const LogosList& v)
{
    const QVariantList qv = qList(v);
    FWD(bool, sBool, fireListEvent(qv), fireListEventAsync(qv, cb));
}
bool TestFullapiQtproxyImpl::fireMapEvent(const LogosMap& v)
{
    const QVariantMap qv = qMap(v);
    FWD(bool, sBool, fireMapEvent(qv), fireMapEventAsync(qv, cb));
}
bool TestFullapiQtproxyImpl::fireTripleEvent(int64_t i, const std::string& s, const std::vector<uint8_t>& b)
{
    const qlonglong qi = qlonglong(i);
    const QString qsv = qs(s);
    const QByteArray qbv = qb(b);
    FWD(bool, sBool, fireTripleEvent(qi, qsv, qbv), fireTripleEventAsync(qi, qsv, qbv, cb));
}

#undef FWD

// ─── Subscribe to the bound provider's events: record + re-emit ──────────────
//
// The re-emitted payload is the value the Qt-typed callback received, so a
// driver watching test_fullapi_qtproxy sees what the QT consumer decoded, not
// what the provider sent. m_lastEvent mirrors the universal proxy's summary
// format so the two consumers' summaries are directly comparable.
//
// The wrapper has no unsubscribe, so re-binding cannot tear the old callbacks
// down. Two guards, both measured as necessary — without them a
// useProvider(cpp) / useProvider(rust) / useProvider(cpp) sequence made every
// subsequent event arrive THREE times, which would multiply every
// event-position cell in the matrix:
//   1. subscribe at most once per provider name;
//   2. every callback drops the delivery unless its provider is still bound, so
//      a previously-bound provider can no longer clobber m_lastEvent.

void TestFullapiQtproxyImpl::subscribeToTarget()
{
    if (m_subscribed.count(m_provider)) return;
    m_subscribed.insert(m_provider);

    const std::string who = m_provider;
    auto stale = [this, who] { return who != m_provider; };
    FullApi api = TARGET;

    api.onStringEvent([this, stale](const QString& v) {
        if (stale()) return;
        m_lastEvent = "stringEvent:" + ss(v);
        stringEvent(ss(v));
    });
    api.onBytesEvent([this, stale](QByteArray v) {
        if (stale()) return;
        m_lastEvent = "bytesEvent:size=" + std::to_string(v.size());
        bytesEvent(sb(v));
    });
    api.onIntEvent([this, stale](qlonglong v) {
        if (stale()) return;
        m_lastEvent = "intEvent:" + ss(QString::number(v));
        intEvent(sInt(v));
    });
    api.onUintEvent([this, stale](qulonglong v) {
        if (stale()) return;
        m_lastEvent = "uintEvent:" + ss(QString::number(v));
        uintEvent(sUint(v));
    });
    api.onDoubleEvent([this, stale](double v) {
        if (stale()) return;
        m_lastEvent = "doubleEvent:" + ss(QString::number(v, 'g', 17));
        doubleEvent(v);
    });
    api.onBoolEvent([this, stale](bool v) {
        if (stale()) return;
        m_lastEvent = std::string("boolEvent:") + (v ? "true" : "false");
        boolEvent(v);
    });
    api.onAnyEvent([this, stale](QVariant v) {
        if (stale()) return;
        m_lastEvent = "anyEvent:" + ss(renderVariant(v));
        anyEvent(sAny(v));
    });
    api.onStringListEvent([this, stale](const QStringList& v) {
        if (stale()) return;
        m_lastEvent = "stringListEvent:size=" + std::to_string(v.size());
        stringListEvent(ssl(v));
    });
    api.onIntListEvent([this, stale](const QList<qlonglong>& v) {
        if (stale()) return;
        m_lastEvent = "intListEvent:size=" + std::to_string(v.size());
        intListEvent(sIntList(v));
    });
    api.onUintListEvent([this, stale](const QList<qulonglong>& v) {
        if (stale()) return;
        m_lastEvent = "uintListEvent:size=" + std::to_string(v.size());
        uintListEvent(sUintList(v));
    });
    api.onDoubleListEvent([this, stale](const QList<double>& v) {
        if (stale()) return;
        m_lastEvent = "doubleListEvent:size=" + std::to_string(v.size());
        doubleListEvent(sDoubleList(v));
    });
    api.onBoolListEvent([this, stale](const QList<bool>& v) {
        if (stale()) return;
        m_lastEvent = "boolListEvent:size=" + std::to_string(v.size());
        boolListEvent(sBoolList(v));
    });
    api.onListEvent([this, stale](const QVariantList& v) {
        if (stale()) return;
        m_lastEvent = "listEvent:size=" + std::to_string(v.size());
        listEvent(sList(v));
    });
    api.onMapEvent([this, stale](const QVariantMap& v) {
        if (stale()) return;
        m_lastEvent = "mapEvent:size=" + std::to_string(v.size());
        mapEvent(sMap(v));
    });
    // The only MULTI-parameter event: the summary carries all three arguments so
    // a subscriber that mixes up positional slots shows here, not just in the
    // re-emit.
    api.onTripleEvent([this, stale](qlonglong i, const QString& s, QByteArray b) {
        if (stale()) return;
        m_lastEvent = "tripleEvent:i=" + ss(QString::number(i)) + ",s=" + ss(s)
                    + ",b=size" + std::to_string(b.size());
        tripleEvent(sInt(i), ss(s), sb(b));
    });
}

#undef TARGET
