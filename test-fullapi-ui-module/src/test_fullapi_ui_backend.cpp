#include "test_fullapi_ui_backend.h"

#include <memory>

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Generated umbrella: LogosModules (behind modules()) with the Qt-typed
// bind_full_api(name) factory from metadata.json#interface_dependencies.
#include "logos_sdk.h"

void TestFullapiUiBackend::onContextReady()
{
    if (boundTarget().isEmpty())
        setBoundTarget(QStringLiteral("test_fullapi_cpp"));
    subscribe();
}

void TestFullapiUiBackend::bindTo(QString name)
{
    setBoundTarget(name);
    subscribe();
}

// Drive one method of EVERY supported type through the bound interface and
// VERIFY each result in the backend. The status PROP is then a single
// deterministic token — "ALL_OK:<provider>" when every type round-trips, or
// "FAIL:<methods>" — so a headless doctest can assert it by EXACT node text (the
// inspector's text-property match is exact, not a substring).
//
// Coverage is the FULL surface: scalars (tstr/int/uint/float64/bool), bstr, any,
// [tstr], {tstr:any}, result, AND every typed array ([int]/[uint]/[float64]/
// [bool]/[any]). The arrays round-trip correctly now that the Qt client packs a
// list arg as one element (logos-cpp-sdk arg-spread fix) — every one is verified
// and in the token. runMethodsAsync() below mirrors this exactly via the async
// wrappers.
void TestFullapiUiBackend::runMethods()
{
    auto api = modules().bind_full_api(boundTarget());
    const QString who = api.whoAmI();

    QVariantMap map;
    map.insert(QStringLiteral("k"), QStringLiteral("v"));

    QStringList fails;
    if (who.isEmpty())                                               fails << QStringLiteral("whoAmI");
    if (api.echoString(QStringLiteral("hi")) != QStringLiteral("hi")) fails << QStringLiteral("echoString");
    if (api.echoInt(42) != 42)                                      fails << QStringLiteral("echoInt");
    if (api.echoUint(7) != 7)                                       fails << QStringLiteral("echoUint");
    if (qAbs(api.echoDouble(2.5) - 2.5) > 1e-9)                     fails << QStringLiteral("echoDouble");
    if (!api.echoBool(true))                                        fails << QStringLiteral("echoBool");
    if (api.echoBytes(QByteArray("\x01\x02\x03", 3)).size() != 3)   fails << QStringLiteral("echoBytes");
    if (api.echoAny(QVariant(QStringLiteral("x"))).toString() != QStringLiteral("x"))
        fails << QStringLiteral("echoAny");
    if (api.echoStringList(QStringList{QStringLiteral("a"), QStringLiteral("b")}).size() != 2)
        fails << QStringLiteral("echoStringList");
    if (api.echoMap(map).size() != 1)                              fails << QStringLiteral("echoMap");
    if (!api.makeResult(true).success)                            fails << QStringLiteral("makeResult");

    // Typed arrays — now verified (each must round-trip its element count).
    if (api.echoIntList(QVariantList{1, 2, 3}).size() != 3)        fails << QStringLiteral("echoIntList");
    if (api.echoUintList(QVariantList{1, 2}).size() != 2)          fails << QStringLiteral("echoUintList");
    if (api.echoDoubleList(QVariantList{1.5, 2.5}).size() != 2)    fails << QStringLiteral("echoDoubleList");
    if (api.echoBoolList(QVariantList{true, false}).size() != 2)   fails << QStringLiteral("echoBoolList");
    if (api.echoList(QVariantList{1, 2, 3}).size() != 3)           fails << QStringLiteral("echoList");

    const QString token = fails.isEmpty()
        ? (QStringLiteral("ALL_OK:") + who)
        : (QStringLiteral("FAIL:") + fails.join(QLatin1Char(',')));
    setStatus(token);
}

// Same FULL-surface verification as runMethods(), but every call goes through the
// ASYNC client wrappers (<method>Async(args, callback)). Each completion fires on
// the UI's Qt event loop (single-threaded, so no locking) and updates a shared
// tally; once all `expected` results have arrived, publish the token
// "ALL_OK_ASYNC:<provider>" (or "FAIL_ASYNC:<methods>"). A headless doctest clicks
// "Run All (Async)" and wait_for's the token — proving the UI → provider ASYNC
// path delivers every type, complementing the synchronous runMethods() token.
void TestFullapiUiBackend::runMethodsAsync()
{
    auto api = modules().bind_full_api(boundTarget());

    struct State {
        int done = 0;
        QStringList fails;
        QString who;
    };
    auto st = std::make_shared<State>();
    // Every type in the full_api surface, exercised asynchronously.
    const int expected = 16;

    // Publish the token once every expected completion has landed.
    auto finish = [this, st, expected]() {
        if (st->done < expected)
            return;
        const QString token = st->fails.isEmpty()
            ? (QStringLiteral("ALL_OK_ASYNC:") + st->who)
            : (QStringLiteral("FAIL_ASYNC:") + st->fails.join(QLatin1Char(',')));
        setStatus(token);
    };
    auto tick = [st, finish](bool ok, const char* name) {
        if (!ok)
            st->fails << QLatin1String(name);
        ++st->done;
        finish();
    };

    QVariantMap map;
    map.insert(QStringLiteral("k"), QStringLiteral("v"));

    // Scalars + bstr + any.
    api.whoAmIAsync([st, tick](QString who) { st->who = who; tick(!who.isEmpty(), "whoAmI"); });
    api.echoStringAsync(QStringLiteral("hi"), [tick](QString r) { tick(r == QStringLiteral("hi"), "echoString"); });
    api.echoIntAsync(42, [tick](int r) { tick(r == 42, "echoInt"); });
    api.echoUintAsync(7, [tick](int r) { tick(r == 7, "echoUint"); });
    api.echoDoubleAsync(2.5, [tick](double r) { tick(qAbs(r - 2.5) <= 1e-9, "echoDouble"); });
    api.echoBoolAsync(true, [tick](bool r) { tick(r, "echoBool"); });
    api.echoBytesAsync(QByteArray("\x01\x02\x03", 3), [tick](QByteArray r) { tick(r.size() == 3, "echoBytes"); });
    api.echoAnyAsync(QVariant(QStringLiteral("x")), [tick](QVariant r) { tick(r.toString() == QStringLiteral("x"), "echoAny"); });

    // Containers.
    api.echoStringListAsync(QStringList{QStringLiteral("a"), QStringLiteral("b")}, [tick](QStringList r) { tick(r.size() == 2, "echoStringList"); });
    api.echoMapAsync(map, [tick](QVariantMap r) { tick(r.size() == 1, "echoMap"); });
    api.makeResultAsync(true, [tick](LogosResult r) { tick(r.success, "makeResult"); });

    // Typed arrays.
    api.echoIntListAsync(QVariantList{1, 2, 3}, [tick](QVariantList r) { tick(r.size() == 3, "echoIntList"); });
    api.echoUintListAsync(QVariantList{1, 2}, [tick](QVariantList r) { tick(r.size() == 2, "echoUintList"); });
    api.echoDoubleListAsync(QVariantList{1.5, 2.5}, [tick](QVariantList r) { tick(r.size() == 2, "echoDoubleList"); });
    api.echoBoolListAsync(QVariantList{true, false}, [tick](QVariantList r) { tick(r.size() == 2, "echoBoolList"); });
    api.echoListAsync(QVariantList{1, 2, 3}, [tick](QVariantList r) { tick(r.size() == 3, "echoList"); });
}

// Fire a couple of events on the bound target; the subscription (below) captures
// them into the lastEvent PROP.
void TestFullapiUiBackend::fireEvents()
{
    auto api = modules().bind_full_api(boundTarget());
    api.fireStringEvent(QStringLiteral("ui-hello"));
    api.fireIntEvent(123);
}

void TestFullapiUiBackend::subscribe()
{
    auto api = modules().bind_full_api(boundTarget());
    api.onStringEvent([this](const QString& v) {
        setLastEvent(QStringLiteral("stringEvent:") + v);
    });
    api.onIntEvent([this](int v) {
        setLastEvent(QStringLiteral("intEvent:") + QString::number(v));
    });
    api.onBytesEvent([this](QByteArray v) {
        setLastEvent(QStringLiteral("bytesEvent:size=") + QString::number(v.size()));
    });
    api.onDoubleEvent([this](double v) {
        setLastEvent(QStringLiteral("doubleEvent:") + QString::number(v));
    });
    api.onBoolEvent([this](bool v) {
        setLastEvent(QStringLiteral("boolEvent:") + (v ? QStringLiteral("true") : QStringLiteral("false")));
    });
    api.onMapEvent([this](const QVariantMap& v) {
        setLastEvent(QStringLiteral("mapEvent:size=") + QString::number(v.size()));
    });
    api.onListEvent([this](const QVariantList& v) {
        setLastEvent(QStringLiteral("listEvent:size=") + QString::number(v.size()));
    });
}
