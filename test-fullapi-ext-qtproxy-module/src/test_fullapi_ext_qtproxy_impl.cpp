#include "test_fullapi_ext_qtproxy_impl.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QList>
#include <QMap>
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
#include "logos_qt_wire.h"     // logos::qt::toWire — the canonical edge

// Generated at build time. metadata.json declares `interface_dependencies`, so
// this umbrella carries `FullApiExt bind_full_api_ext(const QString&)`; it also
// sets `codegen.consumer_api_style: "qt"`, so `FullApiExt` is the QT-TYPED
// wrapper and the umbrella is the origin-bound one — default-constructible,
// holding no LogosAPI, stating this module's own name as the call origin.
#include "logos_sdk.h"

// ─── The two sides of this module ────────────────────────────────────────────
//
// The header is the PROVIDER surface and is std-typed, because the LIDL
// contract — records included — is derived from it and the C exports are
// `logos_module_impl.h`'s. Everything below the line in this file is the
// CONSUMER surface and is Qt-typed, because that is the path being measured. So
// every forwarded method converts once on the way in and once on the way out.
//
// The conversions here are HAND-WRITTEN, and unlike test_fullapi_qtproxy they
// cannot route through `logos::qt::toWire` / `fromWire<T>`: this contract has no
// `any` slot at all, and its Qt spellings — FullApiExt::Blob, QList<QByteArray>,
// QMap<QString, QList<QByteArray>>, std::optional<QString> — are precisely the
// ones QVariant cannot carry (a typed QList matches none of
// qvariantToNlohmann's closed userType() set, which is why the generated
// wrapper emits element loops for them rather than handing them to the codec
// whole). Field-by-field conversion is therefore the only shape available, and
// it is deliberately dumb: no defaulting, no normalising, no reordering.

namespace {

// Shorthand for the three GENERATED record types. They are nested in the
// wrapper class, so they never collide with the std-typed `Blob` / `Wrapper` /
// `Opt` the header declares at global scope for the provider side.
using QBlob    = FullApiExt::Blob;
using QWrapper = FullApiExt::Wrapper;
using QOpt     = FullApiExt::Opt;

// The composite spellings, named once. A `QMap<QString, T>` cannot be written
// inside a macro argument list (the preprocessor splits on its comma), and the
// FWD macro below takes the Qt return type as one argument — so these are what
// the forwarding block names.
using QBlobList        = QList<QBlob>;
using QBlobMap         = QMap<QString, QBlob>;
using QBytesList       = QList<QByteArray>;
using QBytesMap        = QMap<QString, QByteArray>;
using QIntMapT         = QMap<QString, qlonglong>;
using QStringMapT      = QMap<QString, QString>;
using QNestedIntsT     = QList<QList<qlonglong>>;
using QMapOfBytesListsT = QMap<QString, QList<QByteArray>>;
using QOptListT        = QList<QOpt>;

// ─── std <-> Qt, at the provider/consumer boundary ───────────────────────────

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

// The four container shuttles. ONE list loop and ONE map loop per direction,
// each taking its element conversion as a parameter — the same discipline the
// generated code adopted after its two hand-written record loops disagreed.
// A nested type is this same pair applied twice, never a third loop.
template <typename QT, typename ST, typename F>
QList<QT> toQList(const std::vector<ST>& v, F f)
{
    QList<QT> o;
    o.reserve(qsizetype(v.size()));
    for (const ST& e : v) o << f(e);
    return o;
}

template <typename ST, typename QT, typename F>
std::vector<ST> toStdVec(const QList<QT>& v, F f)
{
    std::vector<ST> o;
    o.reserve(size_t(v.size()));
    for (const QT& e : v) o.push_back(f(e));
    return o;
}

template <typename QT, typename ST, typename F>
QMap<QString, QT> toQMap(const std::map<std::string, ST>& v, F f)
{
    QMap<QString, QT> o;
    for (const auto& kv : v) o.insert(qs(kv.first), f(kv.second));
    return o;
}

template <typename ST, typename QT, typename F>
std::map<std::string, ST> toStdMap(const QMap<QString, QT>& v, F f)
{
    std::map<std::string, ST> o;
    for (auto it = v.constBegin(); it != v.constEnd(); ++it)
        o.emplace(ss(it.key()), f(it.value()));
    return o;
}

// ── the records ─────────────────────────────────────────────────────────────
//
// Field by field, and every field named. A memberwise copy is impossible here —
// the two structs are different types with different field spellings — which is
// exactly why a record is worth measuring through a proxy at all.

QBlob qBlob(const Blob& v)
{
    QBlob o;
    o.id = qs(v.id);
    o.n = qulonglong(v.n);
    o.payload = qb(v.payload);
    return o;
}
Blob sBlob(const QBlob& v)
{
    Blob o;
    o.id = ss(v.id);
    o.n = uint64_t(v.n);
    o.payload = sb(v.payload);
    return o;
}

QWrapper qWrapper(const Wrapper& v)
{
    QWrapper o;
    o.inner = qBlob(v.inner);
    o.tags = qsl(v.tags);
    o.blobs = toQList<QBlob>(v.blobs, qBlob);
    return o;
}
Wrapper sWrapper(const QWrapper& v)
{
    Wrapper o;
    o.inner = sBlob(v.inner);
    o.tags = ssl(v.tags);
    o.blobs = toStdVec<Blob>(v.blobs, sBlob);
    return o;
}

// ABSENT STAYS ABSENT, in both directions. `std::optional<T>` on one side and
// `std::optional<U>` on the other is the whole point of the widened mapping:
// before it, a `?tstr` field arrived as a bare QVariant and this conversion
// could not have told an empty optional from an empty string.
QOpt qOpt(const Opt& v)
{
    QOpt o;
    o.required = qs(v.required);
    if (v.maybe) o.maybe = qs(*v.maybe);
    if (v.count) o.count = qulonglong(*v.count);
    if (v.blob)  o.blob = qb(*v.blob);
    return o;
}
Opt sOpt(const QOpt& v)
{
    Opt o;
    o.required = ss(v.required);
    if (v.maybe) o.maybe = ss(*v.maybe);
    if (v.count) o.count = uint64_t(*v.count);
    if (v.blob)  o.blob = sb(*v.blob);
    return o;
}

// ── the composite slots, as NAMED functions ─────────────────────────────────
//
// Named, never inline in a call: `FWD` takes the return conversion as a single
// token, and more importantly the whole set of conversions this module performs
// is then one readable block rather than fifteen scattered lambdas.

QList<QBlob> qBlobList(const std::vector<Blob>& v) { return toQList<QBlob>(v, qBlob); }
std::vector<Blob> sBlobList(const QList<QBlob>& v) { return toStdVec<Blob>(v, sBlob); }

QMap<QString, QBlob> qBlobMap(const std::map<std::string, Blob>& v)
{
    return toQMap<QBlob>(v, qBlob);
}
std::map<std::string, Blob> sBlobMap(const QMap<QString, QBlob>& v)
{
    return toStdMap<Blob>(v, sBlob);
}

QList<QByteArray> qBytesList(const std::vector<std::vector<uint8_t>>& v)
{
    return toQList<QByteArray>(v, qb);
}
std::vector<std::vector<uint8_t>> sBytesList(const QList<QByteArray>& v)
{
    return toStdVec<std::vector<uint8_t>>(v, sb);
}

QMap<QString, QByteArray> qBytesMap(const std::map<std::string, std::vector<uint8_t>>& v)
{
    return toQMap<QByteArray>(v, qb);
}
std::map<std::string, std::vector<uint8_t>> sBytesMap(const QMap<QString, QByteArray>& v)
{
    return toStdMap<std::vector<uint8_t>>(v, sb);
}

qlonglong qi(int64_t v) { return qlonglong(v); }
int64_t   si(qlonglong v) { return int64_t(v); }

QMap<QString, qlonglong> qIntMap(const std::map<std::string, int64_t>& v)
{
    return toQMap<qlonglong>(v, qi);
}
std::map<std::string, int64_t> sIntMap(const QMap<QString, qlonglong>& v)
{
    return toStdMap<int64_t>(v, si);
}

QMap<QString, QString> qStringMap(const std::map<std::string, std::string>& v)
{
    return toQMap<QString>(v, qs);
}
std::map<std::string, std::string> sStringMap(const QMap<QString, QString>& v)
{
    return toStdMap<std::string>(v, ss);
}

QList<qlonglong> qIntList(const std::vector<int64_t>& v) { return toQList<qlonglong>(v, qi); }
std::vector<int64_t> sIntList(const QList<qlonglong>& v) { return toStdVec<int64_t>(v, si); }

QList<QList<qlonglong>> qNestedInts(const std::vector<std::vector<int64_t>>& v)
{
    return toQList<QList<qlonglong>>(v, qIntList);
}
std::vector<std::vector<int64_t>> sNestedInts(const QList<QList<qlonglong>>& v)
{
    return toStdVec<std::vector<int64_t>>(v, sIntList);
}

QMap<QString, QList<QByteArray>> qMapOfBytesLists(
    const std::map<std::string, std::vector<std::vector<uint8_t>>>& v)
{
    return toQMap<QList<QByteArray>>(v, qBytesList);
}
std::map<std::string, std::vector<std::vector<uint8_t>>> sMapOfBytesLists(
    const QMap<QString, QList<QByteArray>>& v)
{
    return toStdMap<std::vector<std::vector<uint8_t>>>(v, sBytesList);
}

QList<QOpt> qOptList(const std::vector<Opt>& v) { return toQList<QOpt>(v, qOpt); }
std::vector<Opt> sOptList(const QList<QOpt>& v) { return toStdVec<Opt>(v, sOpt); }

std::optional<QString> qOptStr(const std::optional<std::string>& v)
{
    if (!v) return std::nullopt;
    return std::optional<QString>(qs(*v));
}
std::optional<std::string> sOptStr(const std::optional<QString>& v)
{
    if (!v) return std::nullopt;
    return std::optional<std::string>(ss(*v));
}

bool sBool(bool v) { return v; }

// The one asymmetric conversion. LogosResult::error is a QVariant, so ABSENT
// and EMPTY are distinguishable; StdLogosResult::error is a std::string, so
// they are not. Recorded rather than smoothed over — it is this module's own
// loss, on the boundary between the Qt consumer wrapper and the std-typed
// surface this module re-exposes.
StdLogosResult sResult(const LogosResult& r)
{
    StdLogosResult o;
    o.success = r.success;
    o.value = logos::qt::toWire(r.value);
    o.error = r.error.isValid() ? r.error.toString().toStdString() : std::string();
    return o;
}

// ─── Rendering ───────────────────────────────────────────────────────────────
//
// syncProbe() and getAsyncProbe() must be comparable to each other and to what
// the provider actually holds, so the rendering has to be LOSSLESS for the
// types this contract carries. QJsonDocument is not an option: it degrades a
// uint64 above int64max to a double, and `Blob.n` is a uint. Hence a
// hand-rolled, type-tagged rendering, in the same vocabulary
// test_fullapi_qtproxy uses so the two proxies' probe strings read alike.
//
//   s:<text>  tstr        i:<n>   int         u:<n>   uint
//   b:<hex>   bstr        B:<t|f> bool        -       empty optional / invalid
//   [..]      list        {k=v}   map         R(..)   result
//   Blob(..) / Wrapper(..) / Opt(..)          records, field by field

QString rStr(const QString& s)   { return "s:" + s; }
QString rBytes(const QByteArray& b) { return "b:" + QString::fromLatin1(b.toHex()); }
QString rInt(qlonglong v)        { return "i:" + QString::number(v); }
QString rUint(qulonglong v)      { return "u:" + QString::number(v); }

template <typename T, typename F>
QString rList(const QList<T>& l, F f)
{
    QStringList parts;
    for (const T& e : l) parts << f(e);
    return "[" + parts.join(",") + "]";
}

template <typename T, typename F>
QString rMap(const QMap<QString, T>& m, F f)
{
    QStringList parts;
    for (auto it = m.constBegin(); it != m.constEnd(); ++it)
        parts << it.key() + "=" + f(it.value());
    return "{" + parts.join(",") + "}";
}

// `-` is reserved for ABSENT and must not be reused for an empty value, or the
// probe hides the very collapse it exists to show: an empty `?tstr` renders
// `-`, a present empty string renders `s:`.
template <typename T, typename F>
QString rOpt(const std::optional<T>& o, F f)
{
    return o ? f(*o) : QString("-");
}

QString rVariant(const QVariant& v)
{
    if (!v.isValid()) return "-";
    switch (v.userType()) {
    case QMetaType::QByteArray: return rBytes(v.toByteArray());
    case QMetaType::QString:    return rStr(v.toString());
    case QMetaType::Bool:       return QString("B:") + (v.toBool() ? "t" : "f");
    case QMetaType::Int:
    case QMetaType::LongLong:   return rInt(v.toLongLong());
    case QMetaType::UInt:
    case QMetaType::ULongLong:  return rUint(v.toULongLong());
    case QMetaType::Double:     return "d:" + QString::number(v.toDouble(), 'g', 17);
    default: break;
    }
    return "?" + QString::fromLatin1(v.typeName()) + ":" + v.toString();
}

QString rBlob(const QBlob& b)
{
    return "Blob(id=" + rStr(b.id) + ",n=" + rUint(b.n)
         + ",payload=" + rBytes(b.payload) + ")";
}

QString rWrapper(const QWrapper& w)
{
    return "Wrapper(inner=" + rBlob(w.inner)
         + ",tags=" + rList<QString>(w.tags, rStr)
         + ",blobs=" + rList<QBlob>(w.blobs, rBlob) + ")";
}

QString rOptRec(const QOpt& o)
{
    return "Opt(required=" + rStr(o.required)
         + ",maybe=" + rOpt<QString>(o.maybe, rStr)
         + ",count=" + rOpt<qulonglong>(o.count, rUint)
         + ",blob=" + rOpt<QByteArray>(o.blob, rBytes) + ")";
}

QString rResult(const LogosResult& r)
{
    return "R(ok=" + QString(r.success ? "t" : "f")
         + ",v=" + rVariant(r.value)
         + ",e=" + rVariant(r.error) + ")";
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
// value — an empty list, a default-constructed record — which is exactly the
// shape of a plausible-looking wrong answer.

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

// A fresh bound wrapper per call. The lp client and its RAII subscriptions live
// in the process-lifetime LpBridge, not in this handle, so a temporary can
// subscribe and a temporary can outlive nothing that matters.
FullApiExt bindTo(const LogosModuleContext& self, const std::string& provider)
{
    return self.modules().bind_full_api_ext(qs(provider));
}

// ── the shared probe inputs ─────────────────────────────────────────────────
//
// Chosen so a value that survives one path and not the other is visible: the
// uint is uint64 max (no signed representation), the int is outside double's
// exact range, and every byte string carries 0x00 / 0x80 / 0xFF.

QByteArray probeBytes()
{
    QByteArray b;
    b.append(char(0x00));
    b.append(char(0x80));
    b.append(char(0xFF));
    return b;
}

const qlonglong  kProbeInt  = Q_INT64_C(-9007199254740993);
const qulonglong kProbeUint = Q_UINT64_C(18446744073709551615);

QBlob probeBlob()
{
    QBlob b;
    b.id = "x";
    b.n = kProbeUint;
    b.payload = probeBytes();
    return b;
}

QWrapper probeWrapper()
{
    QWrapper w;
    w.inner = probeBlob();
    w.tags = QStringList{"a", ""};
    w.blobs = QList<QBlob>{probeBlob()};
    return w;
}

// One field of each optional kind PRESENT and one ABSENT, so a conversion that
// drops presence and one that invents it are different strings.
QOpt probeOpt()
{
    QOpt o;
    o.required = "r";
    o.maybe = QString("m");
    o.count = std::nullopt;
    o.blob = probeBytes();
    return o;
}

const int kAsyncExpected = 15;

} // namespace

#define TARGET bindTo(*this, m_provider)

// ─── Lifecycle + control ─────────────────────────────────────────────────────

void TestFullapiExtQtproxyImpl::onContextReady()
{
    qDebug() << "TestFullapiExtQtproxyImpl: initialized (universal provider, qt consumer)";
    subscribeToTarget();
}

bool TestFullapiExtQtproxyImpl::useProvider(const std::string& moduleName)
{
    m_provider = moduleName;
    subscribeToTarget();
    return true;
}

bool TestFullapiExtQtproxyImpl::useCallMode(const std::string& mode)
{
    // Reject anything else rather than defaulting: a typo that silently left
    // the proxy in sync mode would make the async half of the matrix a
    // duplicate of the sync half — green cells proving nothing.
    if (mode != "sync" && mode != "async") return false;
    m_async = (mode == "async");
    m_lastCallStatus = "ok";
    return true;
}

std::string TestFullapiExtQtproxyImpl::currentCallMode() { return m_async ? "async" : "sync"; }
std::string TestFullapiExtQtproxyImpl::lastCallStatus()  { return m_lastCallStatus; }
std::string TestFullapiExtQtproxyImpl::currentProvider() { return m_provider; }
std::string TestFullapiExtQtproxyImpl::getLastEvent()    { return m_lastEvent; }

// Is this IMAGE's token store the one the outbound lp client reads, and does it
// hold anything? A cdylib has no LogosAPI and no mirroring:
// `logos_module_accept_token` writes straight into the store `lp_client_create`
// reads, which is exactly why `consumer_api_style: "qt"` is safe here.
std::string TestFullapiExtQtproxyImpl::tokenProbe()
{
    auto take = [](char* s) {
        const std::string out = s ? std::string(s) : std::string();
        if (s) lp_string_free(s);
        return out;
    };
    const std::string self = "test_fullapi_ext_qtproxy";
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

// ─── sync vs async, same inputs ──────────────────────────────────────────────
//
// syncProbe() and getAsyncProbe() render the same 15 calls through the sync and
// the async wrapper in the same format, so a driver diffs two strings.

std::string TestFullapiExtQtproxyImpl::syncProbe()
{
    FullApiExt p = TARGET;
    std::map<std::string, QString> r;
    r["whoAmI"]             = rStr(p.whoAmI());
    r["echoBlob"]           = rBlob(p.echoBlob(probeBlob()));
    r["echoWrapper"]        = rWrapper(p.echoWrapper(probeWrapper()));
    r["echoBlobList"]       = rList<QBlob>(p.echoBlobList(QList<QBlob>{probeBlob()}), rBlob);
    r["echoBlobMap"]        = rMap<QBlob>(p.echoBlobMap(QMap<QString, QBlob>{{"k", probeBlob()}}), rBlob);
    r["echoBytesList"]      = rList<QByteArray>(
        p.echoBytesList(QList<QByteArray>{probeBytes(), QByteArray()}), rBytes);
    r["echoBytesMap"]       = rMap<QByteArray>(
        p.echoBytesMap(QMap<QString, QByteArray>{{"k", probeBytes()}}), rBytes);
    r["echoIntMap"]         = rMap<qlonglong>(
        p.echoIntMap(QMap<QString, qlonglong>{{"a", kProbeInt}}), rInt);
    r["echoStringMap"]      = rMap<QString>(
        p.echoStringMap(QMap<QString, QString>{{"a", "x"}, {"b", ""}}), rStr);
    r["echoNestedInts"]     = rList<QList<qlonglong>>(
        p.echoNestedInts(QList<QList<qlonglong>>{QList<qlonglong>{kProbeInt}, QList<qlonglong>{}}),
        [](const QList<qlonglong>& e) { return rList<qlonglong>(e, rInt); });
    r["echoMapOfBytesLists"] = rMap<QList<QByteArray>>(
        p.echoMapOfBytesLists(QMap<QString, QList<QByteArray>>{{"k", QList<QByteArray>{probeBytes()}}}),
        [](const QList<QByteArray>& e) { return rList<QByteArray>(e, rBytes); });
    r["echoOpt"]            = rOptRec(p.echoOpt(probeOpt()));
    r["echoOptList"]        = rList<QOpt>(p.echoOptList(QList<QOpt>{probeOpt()}), rOptRec);
    r["echoOptional"]       = rOpt<QString>(p.echoOptional(std::optional<QString>("hi")), rStr);
    r["fireBlobEvent"]      = QString("B:") + (p.fireBlobEvent(probeBlob()) ? "t" : "f");
    return ss(renderTable(r));
}

void TestFullapiExtQtproxyImpl::recordAsync(const std::string& key, const std::string& rendered)
{
    std::lock_guard<std::mutex> lk(m_asyncMx);
    m_asyncResults[key] = rendered;
    ++m_asyncDone;
}

std::string TestFullapiExtQtproxyImpl::probeAsync()
{
    {
        std::lock_guard<std::mutex> lk(m_asyncMx);
        m_asyncResults.clear();
        m_asyncDone = 0;
    }
    FullApiExt p = TARGET;
    p.whoAmIAsync([this](QString v) { recordAsync("whoAmI", ss(rStr(v))); });
    p.echoBlobAsync(probeBlob(), [this](QBlob v) { recordAsync("echoBlob", ss(rBlob(v))); });
    p.echoWrapperAsync(probeWrapper(), [this](QWrapper v) { recordAsync("echoWrapper", ss(rWrapper(v))); });
    p.echoBlobListAsync(QList<QBlob>{probeBlob()}, [this](QList<QBlob> v) {
        recordAsync("echoBlobList", ss(rList<QBlob>(v, rBlob)));
    });
    p.echoBlobMapAsync(QMap<QString, QBlob>{{"k", probeBlob()}}, [this](QMap<QString, QBlob> v) {
        recordAsync("echoBlobMap", ss(rMap<QBlob>(v, rBlob)));
    });
    p.echoBytesListAsync(QList<QByteArray>{probeBytes(), QByteArray()}, [this](QList<QByteArray> v) {
        recordAsync("echoBytesList", ss(rList<QByteArray>(v, rBytes)));
    });
    p.echoBytesMapAsync(QMap<QString, QByteArray>{{"k", probeBytes()}}, [this](QMap<QString, QByteArray> v) {
        recordAsync("echoBytesMap", ss(rMap<QByteArray>(v, rBytes)));
    });
    p.echoIntMapAsync(QMap<QString, qlonglong>{{"a", kProbeInt}}, [this](QMap<QString, qlonglong> v) {
        recordAsync("echoIntMap", ss(rMap<qlonglong>(v, rInt)));
    });
    p.echoStringMapAsync(QMap<QString, QString>{{"a", "x"}, {"b", ""}}, [this](QMap<QString, QString> v) {
        recordAsync("echoStringMap", ss(rMap<QString>(v, rStr)));
    });
    p.echoNestedIntsAsync(QList<QList<qlonglong>>{QList<qlonglong>{kProbeInt}, QList<qlonglong>{}},
                          [this](QList<QList<qlonglong>> v) {
        recordAsync("echoNestedInts", ss(rList<QList<qlonglong>>(v, [](const QList<qlonglong>& e) {
            return rList<qlonglong>(e, rInt);
        })));
    });
    p.echoMapOfBytesListsAsync(QMap<QString, QList<QByteArray>>{{"k", QList<QByteArray>{probeBytes()}}},
                               [this](QMap<QString, QList<QByteArray>> v) {
        recordAsync("echoMapOfBytesLists", ss(rMap<QList<QByteArray>>(v, [](const QList<QByteArray>& e) {
            return rList<QByteArray>(e, rBytes);
        })));
    });
    p.echoOptAsync(probeOpt(), [this](QOpt v) { recordAsync("echoOpt", ss(rOptRec(v))); });
    p.echoOptListAsync(QList<QOpt>{probeOpt()}, [this](QList<QOpt> v) {
        recordAsync("echoOptList", ss(rList<QOpt>(v, rOptRec)));
    });
    p.echoOptionalAsync(std::optional<QString>("hi"), [this](std::optional<QString> v) {
        recordAsync("echoOptional", ss(rOpt<QString>(v, rStr)));
    });
    p.fireBlobEventAsync(probeBlob(), [this](bool v) {
        recordAsync("fireBlobEvent", std::string("B:") + (v ? "t" : "f"));
    });
    return "started";
}

std::string TestFullapiExtQtproxyImpl::getAsyncProbe()
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
// Every one of the 15 goes through BOTH the sync and the async wrapper,
// selected by useCallMode. Routing the whole case table through both is the
// only way that difference gets measured rather than reasoned about.
//
// FWD keeps the pair adjacent so a method cannot be forwarded in one mode and
// forgotten in the other. `TO_STD` is the boundary conversion for the RETURN,
// always a named function from the block above.

#define FWD(QT_T, TO_STD, CALL, ASYNC_CALL)                                     \
    do {                                                                        \
        FullApiExt _p = TARGET;                                                 \
        if (!m_async) { m_lastCallStatus = "ok-sync"; return TO_STD(_p.CALL); } \
        QT_T _v{};                                                              \
        const bool _ok = awaitAsyncValue<QT_T>(                                 \
            [&](std::function<void(QT_T)> cb) { _p.ASYNC_CALL; }, _v);          \
        m_lastCallStatus = _ok ? "ok-async" : "async-timeout";                  \
        return TO_STD(_v);                                                      \
    } while (0)

std::string TestFullapiExtQtproxyImpl::whoAmI()
{
    FWD(QString, ss, whoAmI(), whoAmIAsync(cb));
}

Blob TestFullapiExtQtproxyImpl::echoBlob(const Blob& v)
{
    const QBlob qv = qBlob(v);
    FWD(QBlob, sBlob, echoBlob(qv), echoBlobAsync(qv, cb));
}

Wrapper TestFullapiExtQtproxyImpl::echoWrapper(const Wrapper& v)
{
    const QWrapper qv = qWrapper(v);
    FWD(QWrapper, sWrapper, echoWrapper(qv), echoWrapperAsync(qv, cb));
}

std::vector<Blob> TestFullapiExtQtproxyImpl::echoBlobList(const std::vector<Blob>& v)
{
    const QList<QBlob> qv = qBlobList(v);
    FWD(QBlobList, sBlobList, echoBlobList(qv), echoBlobListAsync(qv, cb));
}

std::map<std::string, Blob> TestFullapiExtQtproxyImpl::echoBlobMap(
    const std::map<std::string, Blob>& v)
{
    const QMap<QString, QBlob> qv = qBlobMap(v);
    FWD(QBlobMap, sBlobMap, echoBlobMap(qv), echoBlobMapAsync(qv, cb));
}

std::vector<std::vector<uint8_t>> TestFullapiExtQtproxyImpl::echoBytesList(
    const std::vector<std::vector<uint8_t>>& v)
{
    const QList<QByteArray> qv = qBytesList(v);
    FWD(QBytesList, sBytesList, echoBytesList(qv), echoBytesListAsync(qv, cb));
}

std::map<std::string, std::vector<uint8_t>> TestFullapiExtQtproxyImpl::echoBytesMap(
    const std::map<std::string, std::vector<uint8_t>>& v)
{
    const QMap<QString, QByteArray> qv = qBytesMap(v);
    FWD(QBytesMap, sBytesMap, echoBytesMap(qv), echoBytesMapAsync(qv, cb));
}

std::map<std::string, int64_t> TestFullapiExtQtproxyImpl::echoIntMap(
    const std::map<std::string, int64_t>& v)
{
    const QMap<QString, qlonglong> qv = qIntMap(v);
    FWD(QIntMapT, sIntMap, echoIntMap(qv), echoIntMapAsync(qv, cb));
}

std::map<std::string, std::string> TestFullapiExtQtproxyImpl::echoStringMap(
    const std::map<std::string, std::string>& v)
{
    const QMap<QString, QString> qv = qStringMap(v);
    FWD(QStringMapT, sStringMap, echoStringMap(qv), echoStringMapAsync(qv, cb));
}

std::vector<std::vector<int64_t>> TestFullapiExtQtproxyImpl::echoNestedInts(
    const std::vector<std::vector<int64_t>>& v)
{
    const QList<QList<qlonglong>> qv = qNestedInts(v);
    FWD(QNestedIntsT, sNestedInts, echoNestedInts(qv), echoNestedIntsAsync(qv, cb));
}

std::map<std::string, std::vector<std::vector<uint8_t>>>
TestFullapiExtQtproxyImpl::echoMapOfBytesLists(
    const std::map<std::string, std::vector<std::vector<uint8_t>>>& v)
{
    const QMap<QString, QList<QByteArray>> qv = qMapOfBytesLists(v);
    FWD(QMapOfBytesListsT, sMapOfBytesLists,
        echoMapOfBytesLists(qv), echoMapOfBytesListsAsync(qv, cb));
}

Opt TestFullapiExtQtproxyImpl::echoOpt(const Opt& v)
{
    const QOpt qv = qOpt(v);
    FWD(QOpt, sOpt, echoOpt(qv), echoOptAsync(qv, cb));
}

std::vector<Opt> TestFullapiExtQtproxyImpl::echoOptList(const std::vector<Opt>& v)
{
    const QList<QOpt> qv = qOptList(v);
    FWD(QOptListT, sOptList, echoOptList(qv), echoOptListAsync(qv, cb));
}

// The `-> result` slot. The interface declares it that way after
// Both providers declare `-> ?tstr` now, so this wrapper decodes the shape
// each of them actually sends — see the header.
std::optional<std::string> TestFullapiExtQtproxyImpl::echoOptional(
    const std::optional<std::string>& v)
{
    const std::optional<QString> qv = qOptStr(v);
    FWD(std::optional<QString>, sOptStr, echoOptional(qv), echoOptionalAsync(qv, cb));
}

bool TestFullapiExtQtproxyImpl::fireBlobEvent(const Blob& v)
{
    const QBlob qv = qBlob(v);
    FWD(bool, sBool, fireBlobEvent(qv), fireBlobEventAsync(qv, cb));
}

#undef FWD

// ─── Subscribe to the bound provider's event: record + re-emit ───────────────
//
// The re-emitted payload is the value the Qt-typed callback received, so a
// driver watching test_fullapi_ext_qtproxy sees what the QT consumer decoded,
// not what the provider sent.
//
// The wrapper has no unsubscribe, so re-binding cannot tear the old callback
// down. Two guards, both necessary — without them a
// useProvider(cpp)/useProvider(rust)/useProvider(cpp) sequence makes every
// subsequent event arrive three times:
//   1. subscribe at most once per provider name;
//   2. every callback drops the delivery unless its provider is still bound.

void TestFullapiExtQtproxyImpl::subscribeToTarget()
{
    if (m_subscribed.count(m_provider)) return;
    m_subscribed.insert(m_provider);

    const std::string who = m_provider;
    auto stale = [this, who] { return who != m_provider; };
    FullApiExt api = TARGET;

    api.onBlobEvent([this, stale](const QBlob& v) {
        if (stale()) return;
        m_lastEvent = "blobEvent:id=" + ss(v.id)
                    + ",n=" + ss(QString::number(v.n))
                    + ",payload=size" + std::to_string(v.payload.size());
        blobEvent(sBlob(v));
    });
}

#undef TARGET
