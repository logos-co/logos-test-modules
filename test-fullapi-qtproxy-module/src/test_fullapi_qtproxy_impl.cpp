#include "test_fullapi_qtproxy_impl.h"

#include <QDebug>
#include <QMetaType>
#include <QMutexLocker>

// Generated at build time. metadata.json declares `interface_dependencies`, so
// this umbrella carries `FullApi bind_full_api(const QString&)`; the module has
// NO `interface` key, so apiStyle=qt and `FullApi` is the QT-TYPED wrapper.
#include "logos_sdk.h"

// ─── Rendering ───────────────────────────────────────────────────────────────
//
// syncProbe() and getAsyncProbe() must be comparable to each other and to what
// the provider actually holds, so the rendering has to be LOSSLESS for the types
// the matrix cares about. QJsonDocument is not an option: it degrades a uint64
// above int64max to a double, which is precisely the value that discriminates
// the sync table (`_result.toULongLong()`) from the async one
// (`qvariant_cast<qulonglong>(v)`). Hence a hand-rolled, type-tagged rendering.
//
//   s:<text>  tstr        i:<n>   int         u:<n>   uint
//   d:<n>     float64     B:<t|f> bool        b:<hex> bstr
//   [..]      list        {k=v}   map         R(..)   result
//   -         invalid / absent

namespace {

QString renderVariant(const QVariant& v);

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

QString renderTable(const QVariantMap& results)
{
    QStringList parts;
    for (auto it = results.constBegin(); it != results.constEnd(); ++it)
        parts << "\"" + jsonEscape(it.key()) + "\":\"" + jsonEscape(it.value().toString()) + "\"";
    return "{" + parts.join(",") + "}";
}

// The shared probe inputs. Chosen so a value that survives one table and not the
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

// ─── Lifecycle + control ─────────────────────────────────────────────────────

void TestFullapiQtproxyImpl::onInit(LogosAPI* api)
{
    m_api = api;
    delete m_logos;
    m_logos = new LogosModules(api);
    qDebug() << "TestFullapiQtproxyImpl: initialized (apiStyle=qt, bound full_api)";
    subscribeToTarget();
}

FullApi TestFullapiQtproxyImpl::target()
{
    if (!m_logos) m_logos = new LogosModules(logosAPI());
    return m_logos->bind_full_api(m_provider);
}

bool TestFullapiQtproxyImpl::useProvider(const QString& moduleName)
{
    m_provider = moduleName;
    subscribeToTarget();
    return true;
}

QString TestFullapiQtproxyImpl::currentProvider() { return m_provider; }
QString TestFullapiQtproxyImpl::getLastEvent()    { return m_lastEvent; }

QString TestFullapiQtproxyImpl::probeArrays()
{
    FullApi p = target();
    const QVariantList il = p.echoIntList(QVariantList{QVariant::fromValue(qlonglong(1)),
                                                       QVariant::fromValue(qlonglong(2)),
                                                       QVariant::fromValue(qlonglong(3))});
    const QVariantList ul = p.echoUintList(QVariantList{QVariant::fromValue(qulonglong(1)),
                                                        QVariant::fromValue(qulonglong(2))});
    const QVariantList dl = p.echoDoubleList(QVariantList{1.5, 2.5});
    const QVariantList bl = p.echoBoolList(QVariantList{true, false});
    const QStringList  sl = p.echoStringList(QStringList{"a", "b"});
    const QVariantList al = p.echoList(QVariantList{1, 2, 3});
    return QString("intList=%1 uintList=%2 doubleList=%3 boolList=%4 stringList=%5 anyList=%6")
        .arg(il.size()).arg(ul.size()).arg(dl.size())
        .arg(bl.size()).arg(sl.size()).arg(al.size());
}

// ─── sync vs async, same inputs ──────────────────────────────────────────────
//
// The generated tables disagree by construction: the SYNC one converts with
// `_result.toT()`, the ASYNC one with `qvariant_cast<T>(v)` (see
// logos-cpp-sdk/cpp-generator/legacy/generator_lib.cpp, sync ~944-977 vs async
// ~1004-1042). Nothing drove that difference before this module existed.
// syncProbe() and getAsyncProbe() render the same 18 calls through each table in
// the same format, so a driver diffs two strings.

QString TestFullapiQtproxyImpl::syncProbe()
{
    FullApi p = target();
    QVariantMap r;
    r["whoAmI"]         = renderVariant(p.whoAmI());
    r["echoString"]     = renderVariant(p.echoString("zz"));
    r["echoInt"]        = renderVariant(QVariant::fromValue(p.echoInt(kProbeInt)));
    r["echoUint"]       = renderVariant(QVariant::fromValue(p.echoUint(kProbeUint)));
    r["echoDouble"]     = renderVariant(p.echoDouble(2.5));
    r["echoBool"]       = renderVariant(p.echoBool(true));
    r["echoBytes"]      = renderVariant(p.echoBytes(probeBytes()));
    r["echoAny"]        = renderVariant(p.echoAny(QVariant(QString("x"))));
    r["echoStringList"] = renderVariant(p.echoStringList(QStringList{"a", "b"}));
    r["echoIntList"]    = renderVariant(p.echoIntList(QVariantList{QVariant::fromValue(kProbeInt)}));
    r["echoUintList"]   = renderVariant(p.echoUintList(QVariantList{QVariant::fromValue(kProbeUint)}));
    r["echoDoubleList"] = renderVariant(p.echoDoubleList(QVariantList{1.5, 2.5}));
    r["echoBoolList"]   = renderVariant(p.echoBoolList(QVariantList{true, false}));
    r["echoList"]       = renderVariant(p.echoList(QVariantList{1, QString("a"), true}));
    r["echoMap"]        = renderVariant(p.echoMap(QVariantMap{{"k", "v"}}));
    r["echoTriple"]     = renderVariant(p.echoTriple(7, "s", probeBytes()));
    r["makeResult"]     = renderVariant(QVariant::fromValue(p.makeResult(true)));
    p.doVoid();
    r["doVoid"]         = "void:ok";
    return renderTable(r);
}

void TestFullapiQtproxyImpl::recordAsync(const QString& key, const QString& rendered)
{
    QMutexLocker lk(&m_asyncMx);
    m_asyncResults[key] = rendered;
    ++m_asyncDone;
}

QString TestFullapiQtproxyImpl::probeAsync()
{
    {
        QMutexLocker lk(&m_asyncMx);
        m_asyncResults.clear();
        m_asyncDone = 0;
    }
    FullApi p = target();
    p.whoAmIAsync([this](QString v) { recordAsync("whoAmI", renderVariant(v)); });
    p.echoStringAsync("zz", [this](QString v) { recordAsync("echoString", renderVariant(v)); });
    p.echoIntAsync(kProbeInt, [this](qlonglong v) { recordAsync("echoInt", renderVariant(QVariant::fromValue(v))); });
    p.echoUintAsync(kProbeUint, [this](qulonglong v) { recordAsync("echoUint", renderVariant(QVariant::fromValue(v))); });
    p.echoDoubleAsync(2.5, [this](double v) { recordAsync("echoDouble", renderVariant(v)); });
    p.echoBoolAsync(true, [this](bool v) { recordAsync("echoBool", renderVariant(v)); });
    p.echoBytesAsync(probeBytes(), [this](QByteArray v) { recordAsync("echoBytes", renderVariant(v)); });
    p.echoAnyAsync(QVariant(QString("x")), [this](QVariant v) { recordAsync("echoAny", renderVariant(v)); });
    p.echoStringListAsync(QStringList{"a", "b"}, [this](QStringList v) { recordAsync("echoStringList", renderVariant(v)); });
    p.echoIntListAsync(QVariantList{QVariant::fromValue(kProbeInt)}, [this](QVariantList v) { recordAsync("echoIntList", renderVariant(v)); });
    p.echoUintListAsync(QVariantList{QVariant::fromValue(kProbeUint)}, [this](QVariantList v) { recordAsync("echoUintList", renderVariant(v)); });
    p.echoDoubleListAsync(QVariantList{1.5, 2.5}, [this](QVariantList v) { recordAsync("echoDoubleList", renderVariant(v)); });
    p.echoBoolListAsync(QVariantList{true, false}, [this](QVariantList v) { recordAsync("echoBoolList", renderVariant(v)); });
    p.echoListAsync(QVariantList{1, QString("a"), true}, [this](QVariantList v) { recordAsync("echoList", renderVariant(v)); });
    p.echoMapAsync(QVariantMap{{"k", "v"}}, [this](QVariantMap v) { recordAsync("echoMap", renderVariant(v)); });
    p.echoTripleAsync(7, "s", probeBytes(), [this](QString v) { recordAsync("echoTriple", renderVariant(v)); });
    p.makeResultAsync(true, [this](LogosResult v) { recordAsync("makeResult", renderVariant(QVariant::fromValue(v))); });
    p.doVoidAsync([this]() { recordAsync("doVoid", "void:ok"); });
    return "started";
}

QString TestFullapiQtproxyImpl::getAsyncProbe()
{
    QMutexLocker lk(&m_asyncMx);
    if (m_asyncDone < kAsyncExpected)
        return QString("pending=%1").arg(m_asyncDone);
    return renderTable(m_asyncResults);
}

// ─── Forwarded methods ───────────────────────────────────────────────────────

QString TestFullapiQtproxyImpl::whoAmI()                              { return target().whoAmI(); }
QString TestFullapiQtproxyImpl::echoString(const QString& v)          { return target().echoString(v); }
QByteArray TestFullapiQtproxyImpl::echoBytes(const QByteArray& v)     { return target().echoBytes(v); }
qlonglong TestFullapiQtproxyImpl::echoInt(qlonglong v)                { return target().echoInt(v); }
qulonglong TestFullapiQtproxyImpl::echoUint(qulonglong v)             { return target().echoUint(v); }
double TestFullapiQtproxyImpl::echoDouble(double v)                   { return target().echoDouble(v); }
bool TestFullapiQtproxyImpl::echoBool(bool v)                         { return target().echoBool(v); }
QVariant TestFullapiQtproxyImpl::echoAny(const QVariant& v)           { return target().echoAny(v); }
QStringList TestFullapiQtproxyImpl::echoStringList(const QStringList& v) { return target().echoStringList(v); }
QVariantList TestFullapiQtproxyImpl::echoIntList(const QVariantList& v)  { return target().echoIntList(v); }
QVariantList TestFullapiQtproxyImpl::echoUintList(const QVariantList& v) { return target().echoUintList(v); }
QVariantList TestFullapiQtproxyImpl::echoDoubleList(const QVariantList& v) { return target().echoDoubleList(v); }
QVariantList TestFullapiQtproxyImpl::echoBoolList(const QVariantList& v) { return target().echoBoolList(v); }
QVariantList TestFullapiQtproxyImpl::echoList(const QVariantList& v)  { return target().echoList(v); }
QVariantMap TestFullapiQtproxyImpl::echoMap(const QVariantMap& v)     { return target().echoMap(v); }
void TestFullapiQtproxyImpl::doVoid()                                 { target().doVoid(); }
QString TestFullapiQtproxyImpl::echoTriple(qlonglong i, const QString& s, const QByteArray& b)
{
    return target().echoTriple(i, s, b);
}
LogosResult TestFullapiQtproxyImpl::makeResult(bool ok)               { return target().makeResult(ok); }

// ─── Forwarded event triggers ────────────────────────────────────────────────

bool TestFullapiQtproxyImpl::fireStringEvent(const QString& v)        { return target().fireStringEvent(v); }
bool TestFullapiQtproxyImpl::fireBytesEvent(const QByteArray& v)      { return target().fireBytesEvent(v); }
bool TestFullapiQtproxyImpl::fireIntEvent(qlonglong v)                { return target().fireIntEvent(v); }
bool TestFullapiQtproxyImpl::fireUintEvent(qulonglong v)              { return target().fireUintEvent(v); }
bool TestFullapiQtproxyImpl::fireDoubleEvent(double v)                { return target().fireDoubleEvent(v); }
bool TestFullapiQtproxyImpl::fireBoolEvent(bool v)                    { return target().fireBoolEvent(v); }
bool TestFullapiQtproxyImpl::fireAnyEvent(const QVariant& v)          { return target().fireAnyEvent(v); }
bool TestFullapiQtproxyImpl::fireStringListEvent(const QStringList& v){ return target().fireStringListEvent(v); }
bool TestFullapiQtproxyImpl::fireIntListEvent(const QVariantList& v)  { return target().fireIntListEvent(v); }
bool TestFullapiQtproxyImpl::fireUintListEvent(const QVariantList& v) { return target().fireUintListEvent(v); }
bool TestFullapiQtproxyImpl::fireDoubleListEvent(const QVariantList& v) { return target().fireDoubleListEvent(v); }
bool TestFullapiQtproxyImpl::fireBoolListEvent(const QVariantList& v) { return target().fireBoolListEvent(v); }
bool TestFullapiQtproxyImpl::fireListEvent(const QVariantList& v)     { return target().fireListEvent(v); }
bool TestFullapiQtproxyImpl::fireMapEvent(const QVariantMap& v)       { return target().fireMapEvent(v); }
bool TestFullapiQtproxyImpl::fireTripleEvent(qlonglong i, const QString& s, const QByteArray& b)
{
    return target().fireTripleEvent(i, s, b);
}

// ─── Subscribe to the bound provider's events: record + re-emit ──────────────
//
// The re-emitted payload is the value the Qt-typed callback received, so a
// driver watching test_fullapi_qtproxy sees what the QT consumer decoded, not
// what the provider sent. m_lastEvent mirrors the universal proxy's summary
// format so the two consumers' summaries are directly comparable.
//
// LogosAPIClient has no unsubscribe, so re-binding cannot tear the old
// callbacks down. Two guards, both measured as necessary — without them a
// useProvider(cpp) / useProvider(rust) / useProvider(cpp) sequence made every
// subsequent event arrive THREE times, which would multiply every
// event-position cell in the matrix:
//   1. subscribe at most once per provider name;
//   2. every callback drops the delivery unless its provider is still bound, so
//      a previously-bound provider can no longer clobber m_lastEvent.

void TestFullapiQtproxyImpl::subscribeToTarget()
{
    if (m_subscribed.contains(m_provider)) return;
    m_subscribed.insert(m_provider);

    const QString who = m_provider;
    FullApi api = target();
    // Captured by every callback below.
    auto stale = [this, who] { return who != m_provider; };

    api.onStringEvent([this, stale](const QString& v) {
        if (stale()) return;
        m_lastEvent = "stringEvent:" + v;
        emitEvent("stringEvent", QVariantList{QVariant::fromValue(v)});
    });
    api.onBytesEvent([this, stale](QByteArray v) {
        if (stale()) return;
        m_lastEvent = "bytesEvent:size=" + QString::number(v.size());
        emitEvent("bytesEvent", QVariantList{QVariant::fromValue(v)});
    });
    api.onIntEvent([this, stale](qlonglong v) {
        if (stale()) return;
        m_lastEvent = "intEvent:" + QString::number(v);
        emitEvent("intEvent", QVariantList{QVariant::fromValue(v)});
    });
    api.onUintEvent([this, stale](qulonglong v) {
        if (stale()) return;
        m_lastEvent = "uintEvent:" + QString::number(v);
        emitEvent("uintEvent", QVariantList{QVariant::fromValue(v)});
    });
    api.onDoubleEvent([this, stale](double v) {
        if (stale()) return;
        m_lastEvent = "doubleEvent:" + QString::number(v, 'g', 17);
        emitEvent("doubleEvent", QVariantList{QVariant::fromValue(v)});
    });
    api.onBoolEvent([this, stale](bool v) {
        if (stale()) return;
        m_lastEvent = QString("boolEvent:") + (v ? "true" : "false");
        emitEvent("boolEvent", QVariantList{QVariant::fromValue(v)});
    });
    api.onAnyEvent([this, stale](QVariant v) {
        if (stale()) return;
        m_lastEvent = "anyEvent:" + renderVariant(v);
        emitEvent("anyEvent", QVariantList{v});
    });
    api.onStringListEvent([this, stale](const QStringList& v) {
        if (stale()) return;
        m_lastEvent = "stringListEvent:size=" + QString::number(v.size());
        emitEvent("stringListEvent", QVariantList{QVariant::fromValue(v)});
    });
    api.onIntListEvent([this, stale](const QVariantList& v) {
        if (stale()) return;
        m_lastEvent = "intListEvent:size=" + QString::number(v.size());
        emitEvent("intListEvent", QVariantList{QVariant::fromValue(v)});
    });
    api.onUintListEvent([this, stale](const QVariantList& v) {
        if (stale()) return;
        m_lastEvent = "uintListEvent:size=" + QString::number(v.size());
        emitEvent("uintListEvent", QVariantList{QVariant::fromValue(v)});
    });
    api.onDoubleListEvent([this, stale](const QVariantList& v) {
        if (stale()) return;
        m_lastEvent = "doubleListEvent:size=" + QString::number(v.size());
        emitEvent("doubleListEvent", QVariantList{QVariant::fromValue(v)});
    });
    api.onBoolListEvent([this, stale](const QVariantList& v) {
        if (stale()) return;
        m_lastEvent = "boolListEvent:size=" + QString::number(v.size());
        emitEvent("boolListEvent", QVariantList{QVariant::fromValue(v)});
    });
    api.onListEvent([this, stale](const QVariantList& v) {
        if (stale()) return;
        m_lastEvent = "listEvent:size=" + QString::number(v.size());
        emitEvent("listEvent", QVariantList{QVariant::fromValue(v)});
    });
    api.onMapEvent([this, stale](const QVariantMap& v) {
        if (stale()) return;
        m_lastEvent = "mapEvent:size=" + QString::number(v.size());
        emitEvent("mapEvent", QVariantList{QVariant::fromValue(v)});
    });
    // The only MULTI-parameter event: the summary carries all three arguments so
    // a subscriber that mixes up positional slots shows here, not just in the
    // re-emit.
    api.onTripleEvent([this, stale](qlonglong i, const QString& s, QByteArray b) {
        if (stale()) return;
        m_lastEvent = "tripleEvent:i=" + QString::number(i) + ",s=" + s
                    + ",b=size" + QString::number(b.size());
        emitEvent("tripleEvent", QVariantList{QVariant::fromValue(i),
                                              QVariant::fromValue(s),
                                              QVariant::fromValue(b)});
    });
}
