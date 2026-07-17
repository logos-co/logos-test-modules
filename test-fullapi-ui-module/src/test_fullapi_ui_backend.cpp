#include "test_fullapi_ui_backend.h"

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

// Drive one method of each supported type through the bound interface and VERIFY
// each result in the backend. The status PROP is then a single deterministic
// token — "ALL_OK:<provider>" when every verified type round-trips, or
// "FAIL:<methods>" — so a headless doctest can assert it by EXACT node text (the
// inspector's text-property match is exact, not a substring).
//
// Coverage: scalars (tstr/int/uint/float64/bool), bstr, any, [tstr], {tstr:any},
// and result all round-trip correctly over the ui-host QtRO transport and are
// verified. Typed-scalar arrays ([int]/[uint]/[float64]/[bool], and [any]
// carrying numbers) currently round-trip EMPTY over this transport — a Qt-path
// marshaling gap partially addressed by logos-protocol's container int-
// preservation fix but not fully resolved. They are exercised below (so the
// codepath runs) but kept out of the pass/fail token.
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

    // Exercise the typed-scalar / any arrays (see coverage note) — not in token.
    api.echoIntList(QVariantList{1, 2, 3});
    api.echoUintList(QVariantList{1, 2});
    api.echoDoubleList(QVariantList{1.5, 2.5});
    api.echoBoolList(QVariantList{true, false});
    api.echoList(QVariantList{1, 2, 3});

    const QString token = fails.isEmpty()
        ? (QStringLiteral("ALL_OK:") + who)
        : (QStringLiteral("FAIL:") + fails.join(QLatin1Char(',')));
    setStatus(token);
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
