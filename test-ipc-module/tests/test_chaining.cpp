#include "logos_test.h"
#include "logos_mock.h"
#include "test_ipc_module_plugin.h"
#include "logos_api.h"

// ── chainEchoThenReverse ─────────────────────────────────────────────────────

LOGOS_TEST(chainEchoThenReverse_calls_both_modules) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "echo")
        .thenReturn(QVariant(QString("hello")));
    mock.when("test_extlib_module", "reverseString")
        .thenReturn(QVariant(QString("olleh")));

    TestIpcModulePlugin plugin;
    LogosAPI api("test_ipc_module");
    plugin.initLogos(&api);

    QString result = plugin.chainEchoThenReverse("hello");

    LOGOS_ASSERT_EQ(result, QString("olleh"));
    LOGOS_ASSERT(mock.wasCalled("test_basic_module", "echo"));
    LOGOS_ASSERT(mock.wasCalled("test_extlib_module", "reverseString"));
}

LOGOS_TEST(chainEchoThenReverse_passes_echo_output_to_reverse) {
    LogosMockSetup mock;
    // echo returns a transformed string; reverse should receive that output
    mock.when("test_basic_module", "echo")
        .thenReturn(QVariant(QString("echoed_value")));
    mock.when("test_extlib_module", "reverseString")
        .thenReturn(QVariant(QString("final")));

    TestIpcModulePlugin plugin;
    LogosAPI api("test_ipc_module");
    plugin.initLogos(&api);

    plugin.chainEchoThenReverse("input");

    // reverseString should have been called with the output of echo
    LOGOS_ASSERT(mock.wasCalledWith("test_extlib_module", "reverseString",
                                    {QVariant(QString("echoed_value"))}));
}

LOGOS_TEST(chainEchoThenReverse_each_module_called_once) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "echo").thenReturn(QVariant(QString("x")));
    mock.when("test_extlib_module", "reverseString").thenReturn(QVariant(QString("y")));

    TestIpcModulePlugin plugin;
    LogosAPI api("test_ipc_module");
    plugin.initLogos(&api);

    plugin.chainEchoThenReverse("abc");

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

    TestIpcModulePlugin plugin;
    LogosAPI api("test_ipc_module");
    plugin.initLogos(&api);

    QString result = plugin.chainUppercaseThenConcat("hello", "world");

    LOGOS_ASSERT_EQ(result, QString("UPPER UPPER"));
    // uppercaseString called twice (once for each input)
    LOGOS_ASSERT_EQ(mock.callCount("test_extlib_module", "uppercaseString"), 2);
    LOGOS_ASSERT(mock.wasCalled("test_basic_module", "concat"));
}

LOGOS_TEST(chainUppercaseThenConcat_passes_uppercased_values_to_concat) {
    LogosMockSetup mock;
    // Return different values for the two uppercase calls based on call order
    mock.when("test_extlib_module", "uppercaseString")
        .thenReturn(QVariant(QString("MOCKED")));
    mock.when("test_basic_module", "concat")
        .thenReturn(QVariant(QString("MOCKED MOCKED")));

    TestIpcModulePlugin plugin;
    LogosAPI api("test_ipc_module");
    plugin.initLogos(&api);

    plugin.chainUppercaseThenConcat("a", "b");

    // concat should have been called with the two uppercase results
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

    TestIpcModulePlugin plugin;
    LogosAPI api("test_ipc_module");
    plugin.initLogos(&api);

    plugin.chainUppercaseThenConcat("foo", "bar");

    // First call should be "foo", last call (which we can verify) should be "bar"
    QVariantList lastUpperArgs = mock.lastArgs("test_extlib_module", "uppercaseString");
    LOGOS_ASSERT_EQ(lastUpperArgs.size(), 1);
    LOGOS_ASSERT_EQ(lastUpperArgs.at(0).toString(), QString("bar"));
}
