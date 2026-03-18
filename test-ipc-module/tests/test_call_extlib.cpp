#include "logos_test.h"
#include "logos_mock.h"
#include "test_ipc_module_plugin.h"
#include "logos_api.h"

// ── callExtlibReverse ────────────────────────────────────────────────────────

LOGOS_TEST(callExtlibReverse_returns_mocked_string) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "reverseString")
        .thenReturn(QVariant(QString("dlrow olleh")));

    TestIpcModulePlugin plugin;
    LogosAPI api("test_ipc_module");
    plugin.initLogos(&api);

    QString result = plugin.callExtlibReverse("hello world");

    LOGOS_ASSERT_EQ(result, QString("dlrow olleh"));
}

LOGOS_TEST(callExtlibReverse_records_correct_input) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "reverseString").thenReturn(QVariant(QString("")));

    TestIpcModulePlugin plugin;
    LogosAPI api("test_ipc_module");
    plugin.initLogos(&api);

    plugin.callExtlibReverse("abc");

    LOGOS_ASSERT(mock.wasCalledWith("test_extlib_module", "reverseString",
                                    {QVariant(QString("abc"))}));
}

LOGOS_TEST(callExtlibReverse_called_once) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "reverseString").thenReturn(QVariant(QString("")));

    TestIpcModulePlugin plugin;
    LogosAPI api("test_ipc_module");
    plugin.initLogos(&api);

    plugin.callExtlibReverse("test");

    LOGOS_ASSERT_EQ(mock.callCount("test_extlib_module", "reverseString"), 1);
}

// ── callExtlibUppercase ──────────────────────────────────────────────────────

LOGOS_TEST(callExtlibUppercase_returns_mocked_string) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "uppercaseString")
        .thenReturn(QVariant(QString("HELLO")));

    TestIpcModulePlugin plugin;
    LogosAPI api("test_ipc_module");
    plugin.initLogos(&api);

    QString result = plugin.callExtlibUppercase("hello");

    LOGOS_ASSERT_EQ(result, QString("HELLO"));
}

LOGOS_TEST(callExtlibUppercase_records_correct_input) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "uppercaseString").thenReturn(QVariant(QString("")));

    TestIpcModulePlugin plugin;
    LogosAPI api("test_ipc_module");
    plugin.initLogos(&api);

    plugin.callExtlibUppercase("lower");

    LOGOS_ASSERT(mock.wasCalledWith("test_extlib_module", "uppercaseString",
                                    {QVariant(QString("lower"))}));
}

// ── callExtlibCountChars ─────────────────────────────────────────────────────

LOGOS_TEST(callExtlibCountChars_returns_mocked_count) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "countChars").thenReturn(QVariant(5));

    TestIpcModulePlugin plugin;
    LogosAPI api("test_ipc_module");
    plugin.initLogos(&api);

    int result = plugin.callExtlibCountChars("hello");

    LOGOS_ASSERT_EQ(result, 5);
}

LOGOS_TEST(callExtlibCountChars_records_correct_input) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "countChars").thenReturn(QVariant(0));

    TestIpcModulePlugin plugin;
    LogosAPI api("test_ipc_module");
    plugin.initLogos(&api);

    plugin.callExtlibCountChars("testing");

    LOGOS_ASSERT(mock.wasCalledWith("test_extlib_module", "countChars",
                                    {QVariant(QString("testing"))}));
}

// ── wrapperExtlibReverse (generated wrapper) ─────────────────────────────────

LOGOS_TEST(wrapperExtlibReverse_calls_reverseString) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "reverseString")
        .thenReturn(QVariant(QString("reversed")));

    TestIpcModulePlugin plugin;
    LogosAPI api("test_ipc_module");
    plugin.initLogos(&api);

    QString result = plugin.wrapperExtlibReverse("input");

    LOGOS_ASSERT_EQ(result, QString("reversed"));
    LOGOS_ASSERT(mock.wasCalled("test_extlib_module", "reverseString"));
}
