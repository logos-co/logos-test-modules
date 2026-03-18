#include "logos_test.h"
#include "logos_mock.h"
#include "test_ipc_new_api_impl.h"
#include "logos_api.h"

// ── callExtlibReverse ────────────────────────────────────────────────────────

LOGOS_TEST(callExtlibReverse_returns_mocked_string) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "reverseString")
        .thenReturn(QVariant(QString("dlrow olleh")));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    QString result = impl.callExtlibReverse("hello world");

    LOGOS_ASSERT_EQ(result, QString("dlrow olleh"));
}

LOGOS_TEST(callExtlibReverse_records_correct_input) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "reverseString").thenReturn(QVariant(QString("")));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    impl.callExtlibReverse("abc");

    LOGOS_ASSERT(mock.wasCalledWith("test_extlib_module", "reverseString",
                                    {QVariant(QString("abc"))}));
}

LOGOS_TEST(callExtlibReverse_called_once) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "reverseString").thenReturn(QVariant(QString("")));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    impl.callExtlibReverse("test");

    LOGOS_ASSERT_EQ(mock.callCount("test_extlib_module", "reverseString"), 1);
}

// ── callExtlibUppercase ──────────────────────────────────────────────────────

LOGOS_TEST(callExtlibUppercase_returns_mocked_string) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "uppercaseString")
        .thenReturn(QVariant(QString("HELLO")));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    QString result = impl.callExtlibUppercase("hello");

    LOGOS_ASSERT_EQ(result, QString("HELLO"));
}

LOGOS_TEST(callExtlibUppercase_records_correct_input) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "uppercaseString").thenReturn(QVariant(QString("")));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    impl.callExtlibUppercase("lower");

    LOGOS_ASSERT(mock.wasCalledWith("test_extlib_module", "uppercaseString",
                                    {QVariant(QString("lower"))}));
}

// ── callExtlibCountChars ─────────────────────────────────────────────────────

LOGOS_TEST(callExtlibCountChars_returns_mocked_count) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "countChars").thenReturn(QVariant(5));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    int result = impl.callExtlibCountChars("hello");

    LOGOS_ASSERT_EQ(result, 5);
}

LOGOS_TEST(callExtlibCountChars_records_correct_input) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "countChars").thenReturn(QVariant(0));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    impl.callExtlibCountChars("testing");

    LOGOS_ASSERT(mock.wasCalledWith("test_extlib_module", "countChars",
                                    {QVariant(QString("testing"))}));
}

// ── wrapperExtlibReverse (generated wrapper) ─────────────────────────────────

LOGOS_TEST(wrapperExtlibReverse_calls_reverseString) {
    LogosMockSetup mock;
    mock.when("test_extlib_module", "reverseString")
        .thenReturn(QVariant(QString("reversed")));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    QString result = impl.wrapperExtlibReverse("input");

    LOGOS_ASSERT_EQ(result, QString("reversed"));
    LOGOS_ASSERT(mock.wasCalled("test_extlib_module", "reverseString"));
}
