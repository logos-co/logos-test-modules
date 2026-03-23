#include "logos_test.h"
#include "logos_mock.h"
#include "test_ipc_new_api_impl.h"
#include "logos_api.h"

// ── callBasicAddInts ─────────────────────────────────────────────────────────

LOGOS_TEST(callBasicAddInts_returns_mocked_sum) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "addInts").thenReturn(QVariant(30));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    int result = impl.callBasicAddInts(10, 20);

    LOGOS_ASSERT_EQ(result, 30);
}

LOGOS_TEST(callBasicAddInts_records_correct_args) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "addInts").thenReturn(QVariant(12));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    impl.callBasicAddInts(5, 7);

    LOGOS_ASSERT(mock.wasCalled("test_basic_module", "addInts"));
    LOGOS_ASSERT(mock.wasCalledWith("test_basic_module", "addInts",
                                    {QVariant(5), QVariant(7)}));
}

LOGOS_TEST(callBasicAddInts_callCount_is_one) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "addInts").thenReturn(QVariant(0));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    impl.callBasicAddInts(1, 2);

    LOGOS_ASSERT_EQ(mock.callCount("test_basic_module", "addInts"), 1);
}

LOGOS_TEST(callBasicAddInts_called_multiple_times) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "addInts").thenReturn(QVariant(0));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    impl.callBasicAddInts(1, 2);
    impl.callBasicAddInts(3, 4);
    impl.callBasicAddInts(5, 6);

    LOGOS_ASSERT_EQ(mock.callCount("test_basic_module", "addInts"), 3);
}

LOGOS_TEST(callBasicAddInts_last_args_are_recorded) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "addInts").thenReturn(QVariant(0));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    impl.callBasicAddInts(10, 20);
    impl.callBasicAddInts(99, 1);

    QVariantList last = mock.lastArgs("test_basic_module", "addInts");
    LOGOS_ASSERT_EQ(last.size(), 2);
    LOGOS_ASSERT_EQ(last.at(0).toInt(), 99);
    LOGOS_ASSERT_EQ(last.at(1).toInt(), 1);
}

// ── callBasicEcho ────────────────────────────────────────────────────────────

LOGOS_TEST(callBasicEcho_returns_mocked_string) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "echo").thenReturn(QVariant(QString("hello back")));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    QString result = impl.callBasicEcho("hello");

    LOGOS_ASSERT_EQ(result, QString("hello back"));
}

LOGOS_TEST(callBasicEcho_records_input_arg) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "echo").thenReturn(QVariant(QString("")));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    impl.callBasicEcho("test-input");

    LOGOS_ASSERT(mock.wasCalledWith("test_basic_module", "echo",
                                    {QVariant(QString("test-input"))}));
}

// ── callBasicReturnTrue ──────────────────────────────────────────────────────

LOGOS_TEST(callBasicReturnTrue_returns_mocked_true) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "returnTrue").thenReturn(QVariant(true));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    bool result = impl.callBasicReturnTrue();

    LOGOS_ASSERT_TRUE(result);
}

LOGOS_TEST(callBasicReturnTrue_can_be_mocked_false) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "returnTrue").thenReturn(QVariant(false));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    bool result = impl.callBasicReturnTrue();

    LOGOS_ASSERT_FALSE(result);
}

// ── callBasicNoArgs ──────────────────────────────────────────────────────────

LOGOS_TEST(callBasicNoArgs_is_called_with_no_args) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "noArgs").thenReturn(QVariant(QString("ok")));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    impl.callBasicNoArgs();

    LOGOS_ASSERT(mock.wasCalled("test_basic_module", "noArgs"));
    LOGOS_ASSERT(mock.wasCalledWith("test_basic_module", "noArgs", {}));
}

// ── callBasicFiveArgs ────────────────────────────────────────────────────────

LOGOS_TEST(callBasicFiveArgs_records_all_five_args) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "fiveArgs").thenReturn(QVariant(QString("result")));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    impl.callBasicFiveArgs("alpha", 2, true, "delta", 5);

    QVariantList expected = {
        QVariant(QString("alpha")),
        QVariant(2),
        QVariant(true),
        QVariant(QString("delta")),
        QVariant(5)
    };
    LOGOS_ASSERT(mock.wasCalledWith("test_basic_module", "fiveArgs", expected));
}

// ── wrapperBasicEcho (generated wrapper) ────────────────────────────────────

LOGOS_TEST(wrapperBasicEcho_calls_basic_echo) {
    LogosMockSetup mock;
    mock.when("test_basic_module", "echo").thenReturn(QVariant(QString("wrapped")));

    TestIpcNewApiImpl impl;
    LogosAPI api("test_ipc_new_api_module");
    impl.init(&api);

    QString result = impl.wrapperBasicEcho("input");

    LOGOS_ASSERT_EQ(result, QString("wrapped"));
    LOGOS_ASSERT(mock.wasCalled("test_basic_module", "echo"));
}
