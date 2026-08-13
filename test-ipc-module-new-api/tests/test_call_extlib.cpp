#include "logos_test.h"
#include "logos_mock.h"
#include "new_api_fixture.h"

// Calls into test_extlib_module through the generated lp-typed wrappers.
// See test_call_basic.cpp for why numeric arguments are asserted via
// lastArgs() + a typed accessor instead of wasCalledWith().

// ── callExtlibReverse ────────────────────────────────────────────────────────

LOGOS_TEST(callExtlibReverse_returns_mocked_string) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "reverseString")
        .thenReturn(QVariant(QString("dlrow olleh")));

    ImplFixture f;
    std::string result = f->callExtlibReverse("hello world");

    LOGOS_ASSERT_EQ(result, std::string("dlrow olleh"));
}

LOGOS_TEST(callExtlibReverse_records_correct_input) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "reverseString").thenReturn(QVariant(QString("")));

    ImplFixture f;
    f->callExtlibReverse("abc");

    LOGOS_ASSERT(mock.wasCalledWith("test_extlib_module", "reverseString",
                                    {QVariant(QString("abc"))}));
}

LOGOS_TEST(callExtlibReverse_called_once) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "reverseString").thenReturn(QVariant(QString("")));

    ImplFixture f;
    f->callExtlibReverse("test");

    LOGOS_ASSERT_EQ(mock.callCount("test_extlib_module", "reverseString"), 1);
}

// ── callExtlibUppercase ──────────────────────────────────────────────────────

LOGOS_TEST(callExtlibUppercase_returns_mocked_string) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "uppercaseString")
        .thenReturn(QVariant(QString("HELLO")));

    ImplFixture f;
    std::string result = f->callExtlibUppercase("hello");

    LOGOS_ASSERT_EQ(result, std::string("HELLO"));
}

LOGOS_TEST(callExtlibUppercase_records_correct_input) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "uppercaseString").thenReturn(QVariant(QString("")));

    ImplFixture f;
    f->callExtlibUppercase("lower");

    LOGOS_ASSERT(mock.wasCalledWith("test_extlib_module", "uppercaseString",
                                    {QVariant(QString("lower"))}));
}

// ── callExtlibCountChars ─────────────────────────────────────────────────────

LOGOS_TEST(callExtlibCountChars_returns_mocked_count) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "countChars").thenReturn(QVariant(5));

    ImplFixture f;
    int64_t result = f->callExtlibCountChars("hello");

    LOGOS_ASSERT_EQ(result, static_cast<int64_t>(5));
}

LOGOS_TEST(callExtlibCountChars_records_correct_input) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "countChars").thenReturn(QVariant(0));

    ImplFixture f;
    f->callExtlibCountChars("testing");

    LOGOS_ASSERT(mock.wasCalledWith("test_extlib_module", "countChars",
                                    {QVariant(QString("testing"))}));
}

// ── wrapperExtlibReverse ─────────────────────────────────────────────────────

LOGOS_TEST(wrapperExtlibReverse_calls_reverseString) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "reverseString")
        .thenReturn(QVariant(QString("reversed")));

    ImplFixture f;
    std::string result = f->wrapperExtlibReverse("input");

    LOGOS_ASSERT_EQ(result, std::string("reversed"));
    LOGOS_ASSERT(mock.wasCalled("test_extlib_module", "reverseString"));
}
