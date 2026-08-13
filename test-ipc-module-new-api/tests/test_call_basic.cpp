#include "logos_test.h"
#include "logos_mock.h"
#include "new_api_fixture.h"

// Calls into test_basic_module, driven through the generated lp-typed wrappers
// (modules().test_basic_module.<method>) rather than the raw
// LogosAPIClient::invokeRemoteMethod the impl used before it became universal.
//
// The mock still intercepts: LogosMockSetup flips the GLOBAL LogosMode::Mock
// that the transport factory consults, and the lp_* C ABI honours it — see
// logos-protocol's own tests/protocol/test_lp_client.cpp, which mocks
// lp_invoke exactly this way.
//
// ── On asserting arguments ───────────────────────────────────────────────────
// mock.wasCalledWith() compares QVariantList with `==`, i.e. element-wise
// QVariant equality. On the lp path arguments reach MockStore having been
// through JSON, so an integer does NOT necessarily come back as QVariant(int):
// a literal QVariant(5) can compare unequal to the recorded value even though
// the call happened exactly as expected — a silently VACUOUS assertion.
// So numeric arguments are asserted via lastArgs() + a typed accessor, which
// is the idiom logos-protocol's own lp tests use (args[1].toLongLong()).
// String-only argument lists are unambiguous and still use wasCalledWith().

// ── callBasicAddInts ─────────────────────────────────────────────────────────

LOGOS_TEST(callBasicAddInts_returns_mocked_sum) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "addInts").thenReturn(QVariant(30));

    ImplFixture f;
    int64_t result = f->callBasicAddInts(10, 20);

    LOGOS_ASSERT_EQ(result, static_cast<int64_t>(30));
}

LOGOS_TEST(callBasicAddInts_records_correct_args) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "addInts").thenReturn(QVariant(12));

    ImplFixture f;
    f->callBasicAddInts(5, 7);

    LOGOS_ASSERT(mock.wasCalled("test_basic_module", "addInts"));
    QVariantList args = mock.lastArgs("test_basic_module", "addInts");
    LOGOS_ASSERT_EQ(args.size(), 2);
    LOGOS_ASSERT_EQ(args.at(0).toLongLong(), 5LL);
    LOGOS_ASSERT_EQ(args.at(1).toLongLong(), 7LL);
}

LOGOS_TEST(callBasicAddInts_callCount_is_one) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "addInts").thenReturn(QVariant(0));

    ImplFixture f;
    f->callBasicAddInts(1, 2);

    LOGOS_ASSERT_EQ(mock.callCount("test_basic_module", "addInts"), 1);
}

LOGOS_TEST(callBasicAddInts_called_multiple_times) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "addInts").thenReturn(QVariant(0));

    ImplFixture f;
    f->callBasicAddInts(1, 2);
    f->callBasicAddInts(3, 4);
    f->callBasicAddInts(5, 6);

    LOGOS_ASSERT_EQ(mock.callCount("test_basic_module", "addInts"), 3);
}

LOGOS_TEST(callBasicAddInts_last_args_are_recorded) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "addInts").thenReturn(QVariant(0));

    ImplFixture f;
    f->callBasicAddInts(10, 20);
    f->callBasicAddInts(99, 1);

    QVariantList last = mock.lastArgs("test_basic_module", "addInts");
    LOGOS_ASSERT_EQ(last.size(), 2);
    LOGOS_ASSERT_EQ(last.at(0).toLongLong(), 99LL);
    LOGOS_ASSERT_EQ(last.at(1).toLongLong(), 1LL);
}

// ── callBasicEcho ────────────────────────────────────────────────────────────

LOGOS_TEST(callBasicEcho_returns_mocked_string) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "echo").thenReturn(QVariant(QString("hello back")));

    ImplFixture f;
    std::string result = f->callBasicEcho("hello");

    LOGOS_ASSERT_EQ(result, std::string("hello back"));
}

LOGOS_TEST(callBasicEcho_records_input_arg) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "echo").thenReturn(QVariant(QString("")));

    ImplFixture f;
    f->callBasicEcho("test-input");

    LOGOS_ASSERT(mock.wasCalledWith("test_basic_module", "echo",
                                    {QVariant(QString("test-input"))}));
}

// ── callBasicReturnTrue ──────────────────────────────────────────────────────

LOGOS_TEST(callBasicReturnTrue_returns_mocked_true) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "returnTrue").thenReturn(QVariant(true));

    ImplFixture f;
    bool result = f->callBasicReturnTrue();

    LOGOS_ASSERT_TRUE(result);
}

LOGOS_TEST(callBasicReturnTrue_can_be_mocked_false) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "returnTrue").thenReturn(QVariant(false));

    ImplFixture f;
    bool result = f->callBasicReturnTrue();

    LOGOS_ASSERT_FALSE(result);
}

// ── callBasicNoArgs ──────────────────────────────────────────────────────────

LOGOS_TEST(callBasicNoArgs_is_called_with_no_args) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "noArgs").thenReturn(QVariant(QString("ok")));

    ImplFixture f;
    f->callBasicNoArgs();

    LOGOS_ASSERT(mock.wasCalled("test_basic_module", "noArgs"));
    LOGOS_ASSERT_EQ(mock.lastArgs("test_basic_module", "noArgs").size(), 0);
}

// ── callBasicFiveArgs ────────────────────────────────────────────────────────

LOGOS_TEST(callBasicFiveArgs_records_all_five_args) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "fiveArgs").thenReturn(QVariant(QString("result")));

    ImplFixture f;
    f->callBasicFiveArgs("alpha", 2, true, "delta", 5);

    QVariantList args = mock.lastArgs("test_basic_module", "fiveArgs");
    LOGOS_ASSERT_EQ(args.size(), 5);
    LOGOS_ASSERT_EQ(args.at(0).toString(), QString("alpha"));
    LOGOS_ASSERT_EQ(args.at(1).toLongLong(), 2LL);
    LOGOS_ASSERT_EQ(args.at(2).toBool(), true);
    LOGOS_ASSERT_EQ(args.at(3).toString(), QString("delta"));
    LOGOS_ASSERT_EQ(args.at(4).toLongLong(), 5LL);
}

// ── StdLogosResult-returning methods ─────────────────────────────────────────
// New coverage: the old suite asserted nothing about the result-carrying
// methods, and their Qt LogosResult -> StdLogosResult change (QVariant value
// -> nlohmann::json value) is exactly the kind of thing a type swap can get
// silently wrong.

// A LogosResult crosses the wire as its {success, value, error} object (see
// lpPushExpr in the generator), so a mock standing in for a result-returning
// method must return that ENVELOPE, not a bare payload. Returning a bare value
// decodes to success == false — which is how the "missing key" case below
// first passed for entirely the wrong reason.
static QVariant resultEnvelope(const QVariant& value, bool success = true,
                               const QString& error = QString())
{
    QVariantMap env;
    env["success"] = success;
    env["value"]   = value;
    env["error"]   = error;
    return QVariant(env);
}

LOGOS_TEST(callBasicSuccessResult_carries_success_and_value) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "successResult")
        .thenReturn(resultEnvelope(QVariant(QString("operation succeeded"))));

    ImplFixture f;
    StdLogosResult r = f->callBasicSuccessResult();

    LOGOS_ASSERT_TRUE(r.success);
    LOGOS_ASSERT_EQ(r.value.get<std::string>(), std::string("operation succeeded"));
}

LOGOS_TEST(callBasicErrorResult_carries_the_error) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "errorResult")
        .thenReturn(resultEnvelope(QVariant(), /*success=*/false,
                                   QString("deliberate error for testing")));

    ImplFixture f;
    StdLogosResult r = f->callBasicErrorResult();

    LOGOS_ASSERT_FALSE(r.success);
    LOGOS_ASSERT_EQ(r.error, std::string("deliberate error for testing"));
}

LOGOS_TEST(callBasicResultMapField_extracts_the_named_field) {
    LogosMockSetup mock;
    QVariantMap payload;
    payload["name"]  = "test";
    payload["count"] = 42;
    mock.when("test_basic_module", "resultWithMap").thenReturn(resultEnvelope(QVariant(payload)));

    ImplFixture f;

    LOGOS_ASSERT_EQ(f->callBasicResultMapField("name"), std::string("test"));
}

LOGOS_TEST(callBasicResultMapField_missing_key_is_empty) {
    LogosMockSetup mock;
    QVariantMap payload;
    payload["name"] = "test";
    mock.when("test_basic_module", "resultWithMap").thenReturn(resultEnvelope(QVariant(payload)));

    ImplFixture f;

    // Must be empty because the KEY is absent — not because the envelope
    // failed to decode. The sibling test above pins that distinction.
    LOGOS_ASSERT_EQ(f->callBasicResultMapField("nope"), std::string(""));
}

LOGOS_TEST(callBasicResultMapField_is_empty_when_the_call_fails) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "resultWithMap")
        .thenReturn(resultEnvelope(QVariant(), /*success=*/false, QString("boom")));

    ImplFixture f;

    LOGOS_ASSERT_EQ(f->callBasicResultMapField("name"), std::string(""));
}

// ── wrapperBasicEcho ────────────────────────────────────────────────────────

LOGOS_TEST(wrapperBasicEcho_calls_basic_echo) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "echo").thenReturn(QVariant(QString("wrapped")));

    ImplFixture f;
    std::string result = f->wrapperBasicEcho("input");

    LOGOS_ASSERT_EQ(result, std::string("wrapped"));
    LOGOS_ASSERT(mock.wasCalled("test_basic_module", "echo"));
}

// ── Events ───────────────────────────────────────────────────────────────────
// New coverage. `triggeredBasicEvent` used to be emitted dynamically by name
// (emitEvent("triggeredBasicEvent", QVariantList)); it is now a declared
// `logos_events:` event whose body is generated. Assert BOTH halves: the
// downstream call, and the typed emission with its payload.

LOGOS_TEST(triggerBasicEvent_calls_dep_and_emits_typed_event) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "emitTestEvent").thenReturn(QVariant());

    ImplFixture f;
    f->triggerBasicEvent("payload-1");

    LOGOS_ASSERT(mock.wasCalled("test_basic_module", "emitTestEvent"));
    LOGOS_ASSERT(mock.wasCalledWith("test_basic_module", "emitTestEvent",
                                    {QVariant(QString("payload-1"))}));

    LOGOS_ASSERT_EQ(f.events.size(), static_cast<size_t>(1));
    LOGOS_ASSERT_EQ(f.events.at(0).name, std::string("triggeredBasicEvent"));
    LOGOS_ASSERT_TRUE(f.events.at(0).args.is_array());
    LOGOS_ASSERT_EQ(f.events.at(0).args.at(0).get<std::string>(), std::string("payload-1"));
}
