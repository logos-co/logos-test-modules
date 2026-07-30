#include "test_fullapi_qtproxy_impl.h"

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QMetaType>
#include <QMutexLocker>
#include <QThread>

#include "token_manager.h"
#include "logos_protocol.h"

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

// SPIKE. The veneer's State (LpClient + RAII subscriptions) is created once per
// provider and kept here — see LEAK 3 in full_api_veneer.h. `origin` is this
// module's own name, which is what the generated lp umbrella bakes in.
FullApiVeneer TestFullapiQtproxyImpl::veneerTarget()
{
    // LEAK 6 — THE PREREQUISITE. Measured by tokenProbe(): a Qt-style plugin has
    // TWO TokenManager singletons. `logosAPI()` is constructed in the HOST image
    // and handed in, so `informModuleToken` (QtProviderObject) writes the HOST's
    // TokenManager; `lp_client_create`, compiled into the PLUGIN image, reads the
    // plugin's own — which is empty. Every lp call from a Qt plugin therefore
    // presents an empty auth token and capability_module rejects it, so
    // requestModule never mints a per-target token and EVERY call returns a
    // default value. (Measured: "qtTM=100889ec0 lpTM=102fb58a0 same=NO
    // qtCap=yes lpCap=no", and every veneer call returning 0 / "" / [] / {}.)
    //
    // The cdylib backend does not hit this because its generated glue exports
    // `logos_module_accept_token` → `lp_token_save`, which seeds the plugin-side
    // TokenManager (logos-cpp-sdk cpp-generator/experimental/lidl_gen_cdylib.cpp
    // :724-732). The Qt backend has no such hook. THAT is the missing piece, and
    // the two lines below are exactly it — written here so the spike can measure
    // the type behaviour, but they belong in QtProviderObject::informModuleToken.
    if (TokenManager* tm = logosAPI() ? logosAPI()->getTokenManager() : nullptr) {
        const QString cap = tm->getToken(QStringLiteral("capability_module"));
        if (!cap.isEmpty()) lp_token_save("capability_module", cap.toUtf8().constData());
    }

    auto it = m_veneerStates.find(m_provider);
    if (it == m_veneerStates.end()) {
        it = m_veneerStates
                 .emplace(m_provider,
                          std::make_unique<FullApiVeneer::State>(
                              m_provider.toStdString(), std::string("test_fullapi_qtproxy")))
                 .first;
    }
    return FullApiVeneer(it->second.get());
}

bool TestFullapiQtproxyImpl::useWrapper(const QString& which)
{
    if (which != "generated" && which != "veneer") return false;
    m_useVeneer = (which == "veneer");
    subscribeToTarget();
    return true;
}

QString TestFullapiQtproxyImpl::currentWrapper() { return m_useVeneer ? "veneer" : "generated"; }

// Is the TokenManager the Qt client uses the SAME object logos-protocol's
// lp_client_create hands to the client it builds? If not, an lp consumer inside
// a Qt plugin starts with no capability token and every call it makes is
// rejected — which is exactly what the first spike run showed.
QString TestFullapiQtproxyImpl::tokenProbe()
{
    TokenManager* qtTm = logosAPI() ? logosAPI()->getTokenManager() : nullptr;
    TokenManager* lpTm = &TokenManager::instance();
    char* lpCap = lp_token_get("capability_module");
    const QString lpCapS = lpCap ? QString::fromUtf8(lpCap) : QString();
    if (lpCap) lp_string_free(lpCap);
    return QString("qtTM=%1 lpTM=%2 same=%3 qtCap=%4 lpCap=%5 qtProv=%6")
        .arg(reinterpret_cast<quintptr>(qtTm), 0, 16)
        .arg(reinterpret_cast<quintptr>(lpTm), 0, 16)
        .arg(qtTm == lpTm ? "yes" : "NO")
        .arg(qtTm && !qtTm->getToken("capability_module").isEmpty() ? "yes" : "no")
        .arg(lpCapS.isEmpty() ? "no" : "yes")
        .arg(qtTm && !qtTm->getToken(m_provider).isEmpty() ? "yes" : "no");
}

bool TestFullapiQtproxyImpl::useProvider(const QString& moduleName)
{
    m_provider = moduleName;
    subscribeToTarget();
    return true;
}

bool TestFullapiQtproxyImpl::useCallMode(const QString& mode)
{
    // Reject anything else rather than defaulting: a typo that silently left the
    // proxy in sync mode would make the async half of the matrix a duplicate of
    // the sync half — 86 green cells proving nothing.
    if (mode != "sync" && mode != "async") return false;
    m_async = (mode == "async");
    m_lastCallStatus = "ok";
    return true;
}

QString TestFullapiQtproxyImpl::currentCallMode() { return m_async ? "async" : "sync"; }
QString TestFullapiQtproxyImpl::lastCallStatus()  { return m_lastCallStatus; }
QString TestFullapiQtproxyImpl::currentProvider() { return m_provider; }
QString TestFullapiQtproxyImpl::getLastEvent()    { return m_lastEvent; }

bool TestFullapiQtproxyImpl::pumpUntil(const std::function<bool()>& ready)
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

QString TestFullapiQtproxyImpl::probeArrays()
{
    auto run = [&](auto p) -> QString {
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
    };
    if (m_useVeneer) return run(veneerTarget());
    return run(target());
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
    auto run = [&](auto p) -> QString {
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
    };
    if (m_useVeneer) return run(veneerTarget());
    return run(target());
}

// The one cell where the veneer and the generated wrapper disagreed. Renders the
// SAME provider call three ways in one shot so the cause is unambiguous:
//   gen   — generated Qt wrapper (`_result.value<LogosResult>()`)
//   ven   — veneer through the lp wrapper, i.e. via StdLogosResult
//   nostd — veneer with the std hop removed (Qt -> JSON -> invoke -> JSON -> Qt)
QString TestFullapiQtproxyImpl::resultShapeProbe()
{
    const LogosResult a = target().makeResult(true);
    FullApiVeneer v = veneerTarget();
    const LogosResult b = v.makeResult(true);
    const LogosResult c = v.makeResultNoStdHop(true);
    return "gen=" + renderVariant(QVariant::fromValue(a))
         + " ven=" + renderVariant(QVariant::fromValue(b))
         + " nostd=" + renderVariant(QVariant::fromValue(c));
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
    auto run = [&](auto p) {
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
    };
    if (m_useVeneer) run(veneerTarget());
    else             run(target());
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
//
// Every one of the 33 goes through BOTH generated tables, selected by
// useCallMode. The two are not the same code: for the same LIDL type the sync
// wrapper converts with `_result.toT()` and the async one with
// `qvariant_cast<T>(v)` on a valid variant, substituting a DEFAULT on an invalid
// one (logos-cpp-sdk cpp-generator/legacy/generator_lib.cpp). Routing the whole
// case table through both is the only way that difference gets measured rather
// than reasoned about.
//
// FWD/FWD_VOID keep the pair adjacent so a method cannot be forwarded in one
// mode and forgotten in the other — the failure mode this whole module exists to
// remove, one level down.

// SPIKE: `_run` is a GENERIC lambda, so the same text compiles against the
// generated Qt `FullApi` and against `FullApiVeneer`. That the two substitute
// into one body without an edit is itself the first result: the veneer's Qt
// surface is signature-identical to the generated one.
#define FWD(T, CALL, ASYNC_CALL)                                               \
    do {                                                                       \
        auto _run = [&](auto p) -> T {                                         \
            m_lastCallStatus = "ok-sync";                                      \
            if (!m_async) return p.CALL;                                       \
            return awaitAsync<T>([&](std::function<void(T)> cb) { p.ASYNC_CALL; }); \
        };                                                                     \
        if (m_useVeneer) return _run(veneerTarget());                          \
        return _run(target());                                                 \
    } while (0)

QString TestFullapiQtproxyImpl::whoAmI()
{
    FWD(QString, whoAmI(), whoAmIAsync(cb));
}
QString TestFullapiQtproxyImpl::echoString(const QString& v)
{
    FWD(QString, echoString(v), echoStringAsync(v, cb));
}
QByteArray TestFullapiQtproxyImpl::echoBytes(const QByteArray& v)
{
    FWD(QByteArray, echoBytes(v), echoBytesAsync(v, cb));
}
qlonglong TestFullapiQtproxyImpl::echoInt(qlonglong v)
{
    FWD(qlonglong, echoInt(v), echoIntAsync(v, cb));
}
qulonglong TestFullapiQtproxyImpl::echoUint(qulonglong v)
{
    FWD(qulonglong, echoUint(v), echoUintAsync(v, cb));
}
double TestFullapiQtproxyImpl::echoDouble(double v)
{
    FWD(double, echoDouble(v), echoDoubleAsync(v, cb));
}
bool TestFullapiQtproxyImpl::echoBool(bool v)
{
    FWD(bool, echoBool(v), echoBoolAsync(v, cb));
}
QVariant TestFullapiQtproxyImpl::echoAny(const QVariant& v)
{
    FWD(QVariant, echoAny(v), echoAnyAsync(v, cb));
}
QStringList TestFullapiQtproxyImpl::echoStringList(const QStringList& v)
{
    FWD(QStringList, echoStringList(v), echoStringListAsync(v, cb));
}
QVariantList TestFullapiQtproxyImpl::echoIntList(const QVariantList& v)
{
    FWD(QVariantList, echoIntList(v), echoIntListAsync(v, cb));
}
QVariantList TestFullapiQtproxyImpl::echoUintList(const QVariantList& v)
{
    FWD(QVariantList, echoUintList(v), echoUintListAsync(v, cb));
}
QVariantList TestFullapiQtproxyImpl::echoDoubleList(const QVariantList& v)
{
    FWD(QVariantList, echoDoubleList(v), echoDoubleListAsync(v, cb));
}
QVariantList TestFullapiQtproxyImpl::echoBoolList(const QVariantList& v)
{
    FWD(QVariantList, echoBoolList(v), echoBoolListAsync(v, cb));
}
QVariantList TestFullapiQtproxyImpl::echoList(const QVariantList& v)
{
    FWD(QVariantList, echoList(v), echoListAsync(v, cb));
}
QVariantMap TestFullapiQtproxyImpl::echoMap(const QVariantMap& v)
{
    FWD(QVariantMap, echoMap(v), echoMapAsync(v, cb));
}
QString TestFullapiQtproxyImpl::echoTriple(qlonglong i, const QString& s, const QByteArray& b)
{
    FWD(QString, echoTriple(i, s, b), echoTripleAsync(i, s, b, cb));
}
LogosResult TestFullapiQtproxyImpl::makeResult(bool ok)
{
    FWD(LogosResult, makeResult(ok), makeResultAsync(ok, cb));
}

// The one method whose async callback takes no argument, so it cannot go through
// awaitAsync<T>. Split out rather than faked with a dummy T: `void` is the LIDL
// type whose two backends already told the same lie once (registry M2), and a
// dummy return here would be a third place to hide it.
void TestFullapiQtproxyImpl::doVoid()
{
    auto run = [&](auto p) {
        m_lastCallStatus = "ok-sync";
        if (!m_async) { p.doVoid(); return; }
        awaitAsyncVoid([&](std::function<void()> cb) { p.doVoidAsync(cb); });
    };
    if (m_useVeneer) { run(veneerTarget()); return; }
    run(target());
}

// ─── Forwarded event triggers ────────────────────────────────────────────────
//
// These are ordinary `-> bool` methods, so they take the call-mode axis too. The
// EVENT they cause is delivered through the subscription callbacks below, which
// have no sync/async variants — see the header.

bool TestFullapiQtproxyImpl::fireStringEvent(const QString& v)
{
    FWD(bool, fireStringEvent(v), fireStringEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireBytesEvent(const QByteArray& v)
{
    FWD(bool, fireBytesEvent(v), fireBytesEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireIntEvent(qlonglong v)
{
    FWD(bool, fireIntEvent(v), fireIntEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireUintEvent(qulonglong v)
{
    FWD(bool, fireUintEvent(v), fireUintEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireDoubleEvent(double v)
{
    FWD(bool, fireDoubleEvent(v), fireDoubleEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireBoolEvent(bool v)
{
    FWD(bool, fireBoolEvent(v), fireBoolEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireAnyEvent(const QVariant& v)
{
    FWD(bool, fireAnyEvent(v), fireAnyEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireStringListEvent(const QStringList& v)
{
    FWD(bool, fireStringListEvent(v), fireStringListEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireIntListEvent(const QVariantList& v)
{
    FWD(bool, fireIntListEvent(v), fireIntListEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireUintListEvent(const QVariantList& v)
{
    FWD(bool, fireUintListEvent(v), fireUintListEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireDoubleListEvent(const QVariantList& v)
{
    FWD(bool, fireDoubleListEvent(v), fireDoubleListEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireBoolListEvent(const QVariantList& v)
{
    FWD(bool, fireBoolListEvent(v), fireBoolListEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireListEvent(const QVariantList& v)
{
    FWD(bool, fireListEvent(v), fireListEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireMapEvent(const QVariantMap& v)
{
    FWD(bool, fireMapEvent(v), fireMapEventAsync(v, cb));
}
bool TestFullapiQtproxyImpl::fireTripleEvent(qlonglong i, const QString& s, const QByteArray& b)
{
    FWD(bool, fireTripleEvent(i, s, b), fireTripleEventAsync(i, s, b, cb));
}

#undef FWD

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
    // SPIKE: the two wrappers subscribe independently, so the dedup key carries
    // which one. Switching wrappers therefore adds a subscription rather than
    // replacing one — see the note on m_lastEvent below.
    QSet<QString>& seen = m_useVeneer ? m_veneerSubscribed : m_subscribed;
    if (seen.contains(m_provider)) return;
    seen.insert(m_provider);

    const QString who = m_provider;
    const bool viaVeneer = m_useVeneer;
    // Generic over the wrapper type: identical text binds to the generated
    // Qt `FullApi` and to `FullApiVeneer`.
    auto subscribe = [this, who, viaVeneer](auto api) {
    // Captured by every callback below.
    auto stale = [this, who, viaVeneer] { return who != m_provider || viaVeneer != m_useVeneer; };

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
    };  // subscribe

    if (m_useVeneer) subscribe(veneerTarget());
    else             subscribe(target());
}
