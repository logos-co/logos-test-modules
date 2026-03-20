#ifndef TEST_IPC_MODULE_INTERFACE_H
#define TEST_IPC_MODULE_INTERFACE_H

#include <QObject>
#include <QString>
#include "interface.h"
#include "logos_types.h"

class TestIpcModuleInterface : public PluginInterface
{
public:
    virtual ~TestIpcModuleInterface() = default;

    // ── Calls to test_basic_module ───────────────────────────────────────────

    // Call echo(QString) -> QString via raw invokeRemoteMethod
    Q_INVOKABLE virtual QString callBasicEcho(const QString& input) = 0;

    // Call addInts(int, int) -> int via raw invokeRemoteMethod
    Q_INVOKABLE virtual int callBasicAddInts(int a, int b) = 0;

    // Call returnTrue() -> bool via raw invokeRemoteMethod
    Q_INVOKABLE virtual bool callBasicReturnTrue() = 0;

    // Call noArgs() -> QString (0 args)
    Q_INVOKABLE virtual QString callBasicNoArgs() = 0;

    // Call fiveArgs() -> QString (5 args)
    Q_INVOKABLE virtual QString callBasicFiveArgs(const QString& a, int b, bool c, const QString& d, int e) = 0;

    // Call successResult() -> LogosResult
    Q_INVOKABLE virtual LogosResult callBasicSuccessResult() = 0;

    // Call errorResult() -> LogosResult
    Q_INVOKABLE virtual LogosResult callBasicErrorResult() = 0;

    // Call resultWithMap() and extract a value
    Q_INVOKABLE virtual QString callBasicResultMapField(const QString& key) = 0;

    // ── Calls to test_extlib_module ──────────────────────────────────────────

    // Call reverseString(QString) -> QString
    Q_INVOKABLE virtual QString callExtlibReverse(const QString& input) = 0;

    // Call uppercaseString(QString) -> QString
    Q_INVOKABLE virtual QString callExtlibUppercase(const QString& input) = 0;

    // Call countChars(QString) -> int
    Q_INVOKABLE virtual int callExtlibCountChars(const QString& input) = 0;

    // ── Cross-module chaining ────────────────────────────────────────────────

    // Chain: echo from basic -> reverse from extlib
    Q_INVOKABLE virtual QString chainEchoThenReverse(const QString& input) = 0;

    // Chain: uppercase from extlib -> concat from basic
    Q_INVOKABLE virtual QString chainUppercaseThenConcat(const QString& a, const QString& b) = 0;

    // ── Generated wrappers (LogosModules) ────────────────────────────────────

    // Call via generated type-safe wrapper: test_basic_module.echo
    Q_INVOKABLE virtual QString wrapperBasicEcho(const QString& input) = 0;

    // Call via generated type-safe wrapper: test_extlib_module.reverseString
    Q_INVOKABLE virtual QString wrapperExtlibReverse(const QString& input) = 0;

    // ── Events ───────────────────────────────────────────────────────────────

    // Trigger an event on test_basic_module and report
    Q_INVOKABLE virtual void triggerBasicEvent(const QString& data) = 0;

    // ── Async calls ───────────────────────────────────────────────────────────

    // Async echo via raw invokeRemoteMethodAsync, blocks with QEventLoop until callback
    Q_INVOKABLE virtual QString asyncCallBasicEcho(const QString& input) = 0;

    // Async addInts via raw invokeRemoteMethodAsync (exercises multi-arg async overload)
    Q_INVOKABLE virtual int asyncCallBasicAddInts(int a, int b) = 0;

    // Async reverseString via raw invokeRemoteMethodAsync (cross-module to extlib)
    Q_INVOKABLE virtual QString asyncCallExtlibReverse(const QString& input) = 0;

    // Async echo via generated type-safe echoAsync() wrapper
    Q_INVOKABLE virtual QString asyncWrapperBasicEcho(const QString& input) = 0;
};

#define TestIpcModuleInterface_iid "org.logos.TestIpcModuleInterface"
Q_DECLARE_INTERFACE(TestIpcModuleInterface, TestIpcModuleInterface_iid)

#endif // TEST_IPC_MODULE_INTERFACE_H
