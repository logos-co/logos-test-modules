#include "logos_test.h"
#include "logos_mock.h"
#include "new_api_fixture.h"

// Cross-module chaining: the output of one dep call feeds the next. These are
// the tests that would go quiet first if the mock ever stopped intercepting
// the lp path — a non-intercepting call returns a default-constructed value,
// so "passes X to Y" would compare empty against empty. Each one therefore
// asserts the RETURNED value as well as the recorded argument.

// ── chainEchoThenReverse ─────────────────────────────────────────────────────

LOGOS_TEST(chainEchoThenReverse_calls_both_modules) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "echo")
        .thenReturn(QVariant(QString("hello")));
    mock.when("test_extlib_module", "reverseString")
        .thenReturn(QVariant(QString("olleh")));

    ImplFixture f;
    std::string result = f->chainEchoThenReverse("hello");

    LOGOS_ASSERT_EQ(result, std::string("olleh"));
    LOGOS_ASSERT(mock.wasCalled("test_basic_module", "echo"));
    LOGOS_ASSERT(mock.wasCalled("test_extlib_module", "reverseString"));
}

LOGOS_TEST(chainEchoThenReverse_passes_echo_output_to_reverse) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "echo")
        .thenReturn(QVariant(QString("echoed_value")));
    mock.when("test_extlib_module", "reverseString")
        .thenReturn(QVariant(QString("final")));

    ImplFixture f;
    f->chainEchoThenReverse("input");

    LOGOS_ASSERT(mock.wasCalledWith("test_extlib_module", "reverseString",
                                    {QVariant(QString("echoed_value"))}));
}

LOGOS_TEST(chainEchoThenReverse_each_module_called_once) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "echo").thenReturn(QVariant(QString("x")));
    mock.when("test_extlib_module", "reverseString").thenReturn(QVariant(QString("y")));

    ImplFixture f;
    f->chainEchoThenReverse("abc");

    LOGOS_ASSERT_EQ(mock.callCount("test_basic_module", "echo"), 1);
    LOGOS_ASSERT_EQ(mock.callCount("test_extlib_module", "reverseString"), 1);
}

// ── chainUppercaseThenConcat ─────────────────────────────────────────────────

LOGOS_TEST(chainUppercaseThenConcat_calls_three_methods) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "uppercaseString")
        .thenReturn(QVariant(QString("UPPER")));
    mock.when("test_basic_module", "concat")
        .thenReturn(QVariant(QString("UPPER UPPER")));

    ImplFixture f;
    std::string result = f->chainUppercaseThenConcat("hello", "world");

    LOGOS_ASSERT_EQ(result, std::string("UPPER UPPER"));
    LOGOS_ASSERT_EQ(mock.callCount("test_extlib_module", "uppercaseString"), 2);
    LOGOS_ASSERT(mock.wasCalled("test_basic_module", "concat"));
}

LOGOS_TEST(chainUppercaseThenConcat_passes_uppercased_values_to_concat) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "uppercaseString")
        .thenReturn(QVariant(QString("MOCKED")));
    mock.when("test_basic_module", "concat")
        .thenReturn(QVariant(QString("MOCKED MOCKED")));

    ImplFixture f;
    f->chainUppercaseThenConcat("a", "b");

    LOGOS_ASSERT(mock.wasCalledWith("test_basic_module", "concat",
                                    {QVariant(QString("MOCKED")),
                                     QVariant(QString("MOCKED"))}));
}

LOGOS_TEST(chainUppercaseThenConcat_records_original_inputs_for_uppercase) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "uppercaseString")
        .thenReturn(QVariant(QString("X")));
    mock.when("test_basic_module", "concat")
        .thenReturn(QVariant(QString("X X")));

    ImplFixture f;
    f->chainUppercaseThenConcat("foo", "bar");

    QVariantList lastUpperArgs = mock.lastArgs("test_extlib_module", "uppercaseString");
    LOGOS_ASSERT_EQ(lastUpperArgs.size(), 1);
    LOGOS_ASSERT_EQ(lastUpperArgs.at(0).toString(), QString("bar"));
}
