#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# Logos Test Modules — Integration Test Suite
#
# Exercises every API type and combination in the test modules using logoscore.
# Usage: run_tests.sh <logoscore> <basic-lib-dir> <extlib-lib-dir> <ipc-lib-dir>
# ─────────────────────────────────────────────────────────────────────────────
set -uo pipefail

LOGOSCORE="${1:?Usage: run_tests.sh <logoscore> <basic-lib-dir> <extlib-lib-dir> <ipc-lib-dir>}"
BASIC_DIR="${2:?}"
EXTLIB_DIR="${3:?}"
IPC_DIR="${4:?}"

# Per-call timeout (seconds) — guard against total hangs.
CALL_TIMEOUT=30

# Require --quit-on-finish support (added in logos-liblogos after initial release).
# Without this flag logoscore hangs after method calls, making it impossible to
# distinguish success from failure reliably.
if "$LOGOSCORE" --help 2>&1 | grep -q "quit-on-finish"; then
    QUIT_FLAG="--quit-on-finish"
    echo "  quit-flag : --quit-on-finish (detected)"
else
    echo "ERROR: logoscore does not support --quit-on-finish." >&2
    echo "       This flag is required for reliable test execution." >&2
    echo "       Please update logos-liblogos to a version that includes it." >&2
    exit 1
fi

PASS=0
FAIL=0
SKIP=0
TOTAL=0
FAILURES=""

# ── Helpers ──────────────────────────────────────────────────────────────────

# assert_call: run a logoscore call, check exit code 0 and stdout contains pattern
#   $1 = test name
#   $2 = expected substring in stdout ("" to skip output check)
#   $3... = logoscore arguments
assert_call() {
    local name="$1"; shift
    local expected="$1"; shift
    TOTAL=$((TOTAL + 1))

    # shellcheck disable=SC2086
    printf "        cmd: timeout %s %s %s %s\n" "$CALL_TIMEOUT" "$LOGOSCORE" "$QUIT_FLAG" "$*"
    local output stderr_file rc
    stderr_file=$(mktemp)
    # shellcheck disable=SC2086
    output=$(timeout "$CALL_TIMEOUT" "$LOGOSCORE" $QUIT_FLAG "$@" 2>"$stderr_file") && rc=0 || rc=$?

    if [[ $rc -eq 0 ]]; then
        rm -f "$stderr_file"
        if [[ -z "$expected" ]] || printf '%s' "$output" | grep -qF "$expected"; then
            PASS=$((PASS + 1))
            printf "  PASS  %s\n" "$name"
            return 0
        else
            FAIL=$((FAIL + 1))
            printf "  FAIL  %s  (expected '%s' in output, got: '%s')\n" "$name" "$expected" "$output"
            FAILURES="${FAILURES}  FAIL  ${name}: expected '${expected}', got '${output}'\n"
            return 1
        fi
    else
        FAIL=$((FAIL + 1))
        local stderr_out
        stderr_out=$(cat "$stderr_file" 2>/dev/null)
        printf "  FAIL  %s  (logoscore exit code %d)\n" "$name" "$rc"
        if [[ $FAIL -le 1 ]] && [[ -n "$stderr_out" ]]; then
            printf "        === stderr start ===\n"
            printf "%s\n" "$stderr_out"
            printf "        === stderr end ===\n"
        fi
        FAILURES="${FAILURES}  FAIL  ${name}: logoscore exit code ${rc}\n"
        rm -f "$stderr_file"
        return 1
    fi
}

# assert_call_fails: run a logoscore call, expect non-zero exit code
assert_call_fails() {
    local name="$1"; shift
    TOTAL=$((TOTAL + 1))

    # shellcheck disable=SC2086
    printf "        cmd: timeout %s %s %s %s\n" "$CALL_TIMEOUT" "$LOGOSCORE" "$QUIT_FLAG" "$*"
    local rc
    # shellcheck disable=SC2086
    timeout "$CALL_TIMEOUT" "$LOGOSCORE" $QUIT_FLAG "$@" >/dev/null 2>&1 && rc=0 || rc=$?

    if [[ $rc -eq 0 ]]; then
        FAIL=$((FAIL + 1))
        printf "  FAIL  %s  (expected failure, but got exit 0)\n" "$name"
        FAILURES="${FAILURES}  FAIL  ${name}: expected failure, got success\n"
        return 1
    else
        PASS=$((PASS + 1))
        printf "  PASS  %s  (correctly failed)\n" "$name"
        return 0
    fi
}

skip_test() {
    SKIP=$((SKIP + 1))
    printf "  SKIP  %s  (%s)\n" "$1" "$2"
}

# Shorthands for each module
test_basic() {
    assert_call "$1" "$2" -m "$BASIC_DIR" -l test_basic_module -c "$3"
}
test_extlib() {
    assert_call "$1" "$2" -m "$EXTLIB_DIR" -l test_extlib_module -c "$3"
}
test_ipc() {
    assert_call "$1" "$2" \
        -m "$BASIC_DIR" -m "$EXTLIB_DIR" -m "$IPC_DIR" \
        -l test_ipc_module -c "$3"
}

# ── Banner ───────────────────────────────────────────────────────────────────

echo "================================================================="
echo " Logos Test Modules -- Integration Tests"
echo "================================================================="
echo ""
echo "  logoscore : $LOGOSCORE"
echo "  basic     : $BASIC_DIR"
echo "  extlib    : $EXTLIB_DIR"
echo "  ipc       : $IPC_DIR"
echo ""

# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 1: test_basic_module (standalone, no IPC)
# ═════════════════════════════════════════════════════════════════════════════

echo "-----------------------------------------------------------------"
echo " test_basic_module"
echo "-----------------------------------------------------------------"

# ── Return type: bool ────────────────────────────────────────────────────────
echo ""
echo "  -- Return type: bool --"
test_basic "returnTrue()"       "Result: true"   "test_basic_module.returnTrue()"
test_basic "returnFalse()"      "Result: false"  "test_basic_module.returnFalse()"
test_basic "isPositive(5)"      "Result: true"   "test_basic_module.isPositive(5)"
test_basic "isPositive(0)"      "Result: false"  "test_basic_module.isPositive(0)"
test_basic "isPositive(-3)"     "Result: false"  "test_basic_module.isPositive(-3)"

# ── Return type: int ─────────────────────────────────────────────────────────
echo ""
echo "  -- Return type: int --"
test_basic "returnInt()"        "Result: 42"  "test_basic_module.returnInt()"
test_basic "addInts(3, 4)"      "Result: 7"   "test_basic_module.addInts(3, 4)"
test_basic "addInts(0, 0)"      "Result: 0"   "test_basic_module.addInts(0, 0)"
test_basic "addInts(-5, 10)"    "Result: 5"   "test_basic_module.addInts(-5, 10)"
test_basic "stringLength(hello)" "Result: 5"  "test_basic_module.stringLength(hello)"
skip_test  "stringLength()"     "logoscore cannot call 1-arg method with 0 args"

# ── Return type: QString ─────────────────────────────────────────────────────
echo ""
echo "  -- Return type: QString --"
test_basic "returnString()"     "Result: test_basic_module"  "test_basic_module.returnString()"
test_basic "echo(hello)"        "Result: hello"              "test_basic_module.echo(hello)"
test_basic "echo(world)"        "Result: world"              "test_basic_module.echo(world)"
test_basic "concat(foo, bar)"   "Result: foobar"             "test_basic_module.concat(foo, bar)"
skip_test  "concat(, )"        "logoscore cannot pass empty args"

# ── Return type: LogosResult ─────────────────────────────────────────────────
echo ""
echo "  -- Return type: LogosResult --"
test_basic "successResult()"    "Method call successful"     "test_basic_module.successResult()"
test_basic "errorResult()"      "Method call successful"     "test_basic_module.errorResult()"
test_basic "resultWithMap()"    "Method call successful"     "test_basic_module.resultWithMap()"
test_basic "resultWithList()"   "Method call successful"     "test_basic_module.resultWithList()"
test_basic "validateInput(hi)"  "Method call successful"     "test_basic_module.validateInput(hi)"

# ── Return type: QVariant ────────────────────────────────────────────────────
echo ""
echo "  -- Return type: QVariant --"
test_basic "returnVariantInt()"    "Result: 99"              "test_basic_module.returnVariantInt()"
test_basic "returnVariantString()" "Result: variant_string"  "test_basic_module.returnVariantString()"
test_basic "returnVariantMap()"    "Method call successful"  "test_basic_module.returnVariantMap()"
test_basic "returnVariantList()"   "Method call successful"  "test_basic_module.returnVariantList()"

# ── Return type: QJsonArray ──────────────────────────────────────────────────
echo ""
echo "  -- Return type: QJsonArray --"
test_basic "returnJsonArray()"        "Method call successful"  "test_basic_module.returnJsonArray()"
test_basic "makeJsonArray(x, y)"      "Method call successful"  "test_basic_module.makeJsonArray(x, y)"

# ── Return type: QStringList ─────────────────────────────────────────────────
echo ""
echo "  -- Return type: QStringList --"
test_basic "returnStringList()"       "Method call successful"  "test_basic_module.returnStringList()"
skip_test  "splitString(a,b,c)"       "commas in arg value parsed as arg separators by logoscore"

# ── Parameter types ──────────────────────────────────────────────────────────
echo ""
echo "  -- Parameter types --"
test_basic "echoInt(42)"           "Result: 42"     "test_basic_module.echoInt(42)"
test_basic "echoInt(0)"            "Result: 0"      "test_basic_module.echoInt(0)"
test_basic "echoInt(-7)"           "Result: -7"     "test_basic_module.echoInt(-7)"
test_basic "echoBool(true)"        "Result: true"   "test_basic_module.echoBool(true)"
test_basic "echoBool(false)"       "Result: false"  "test_basic_module.echoBool(false)"
skip_test  "joinStrings(QStringList)"    "logoscore cannot pass QStringList params"
skip_test  "byteArraySize(QByteArray)"   "logoscore cannot pass QByteArray params"
skip_test  "urlToString(QUrl)"           "logoscore cannot pass QUrl params"

# ── Argument counts 0–5 ─────────────────────────────────────────────────────
echo ""
echo "  -- Argument counts 0-5 --"
test_basic "noArgs()"                               "Result: noArgs()"                             "test_basic_module.noArgs()"
test_basic "oneArg(x)"                              "Result: oneArg(x)"                            "test_basic_module.oneArg(x)"
test_basic "twoArgs(x, 1)"                          "Result: twoArgs(x, 1)"                        "test_basic_module.twoArgs(x, 1)"
test_basic "threeArgs(x, 1, true)"                  "Result: threeArgs(x, 1, true)"                "test_basic_module.threeArgs(x, 1, true)"
test_basic "fourArgs(x, 1, true, y)"                "Result: fourArgs(x, 1, true, y)"              "test_basic_module.fourArgs(x, 1, true, y)"
test_basic "fiveArgs(x, 1, true, y, 2)"             "Result: fiveArgs(x, 1, true, y, 2)"           "test_basic_module.fiveArgs(x, 1, true, y, 2)"

# ── Void methods ─────────────────────────────────────────────────────────────
echo ""
echo "  -- Void methods (logoscore returns non-zero for void, testing no crash) --"
skip_test  "doNothing()"            "void return → invalid QVariant → logoscore exit 1"
skip_test  "doNothingWithArgs(a,1)" "void return → invalid QVariant → logoscore exit 1"

# ── Events (fire-and-forget via logoscore, just test no crash) ───────────────
echo ""
echo "  -- Events --"
skip_test  "emitTestEvent(data)"        "void return → invalid QVariant → logoscore exit 1"
skip_test  "emitMultiArgEvent(ev, 5)"   "void return → invalid QVariant → logoscore exit 1"


# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 2: test_extlib_module (external C library wrapper)
# ═════════════════════════════════════════════════════════════════════════════

echo ""
echo "-----------------------------------------------------------------"
echo " test_extlib_module"
echo "-----------------------------------------------------------------"

echo ""
echo "  -- String operations via libstrutil --"
test_extlib "reverseString(hello)"      "Result: olleh"   "test_extlib_module.reverseString(hello)"
test_extlib "reverseString(abc)"        "Result: cba"     "test_extlib_module.reverseString(abc)"
test_extlib "reverseString(a)"          "Result: a"       "test_extlib_module.reverseString(a)"
test_extlib "uppercaseString(hello)"    "Result: HELLO"   "test_extlib_module.uppercaseString(hello)"
test_extlib "uppercaseString(FooBar)"   "Result: FOOBAR"  "test_extlib_module.uppercaseString(FooBar)"
test_extlib "lowercaseString(HELLO)"    "Result: hello"   "test_extlib_module.lowercaseString(HELLO)"
test_extlib "lowercaseString(FooBar)"   "Result: foobar"  "test_extlib_module.lowercaseString(FooBar)"

echo ""
echo "  -- Counting --"
test_extlib "countChars(hello)"         "Result: 5"    "test_extlib_module.countChars(hello)"
skip_test   "countChars()"             "logoscore cannot call 1-arg method with 0 args"
test_extlib "countChar(hello, l)"       "Result: 2"    "test_extlib_module.countChar(hello, l)"
test_extlib "countChar(hello, z)"       "Result: 0"    "test_extlib_module.countChar(hello, z)"
test_extlib "countChar(aabaa, a)"       "Result: 4"    "test_extlib_module.countChar(aabaa, a)"

echo ""
echo "  -- Library version --"
test_extlib "libVersion()"              "Result: 1.0.0"  "test_extlib_module.libVersion()"


# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 3: test_ipc_module (inter-module communication)
# ═════════════════════════════════════════════════════════════════════════════

echo ""
echo "-----------------------------------------------------------------"
echo " test_ipc_module (requires all 3 modules)"
echo "-----------------------------------------------------------------"

# NOTE: The capability_module must be bundled with logoscore for IPC to work.
# logoscore auto-discovers it from its default modules dir (../modules relative
# to the binary). When user-specified -m dirs are also provided, both the default
# and user dirs are scanned.

# ── Calls to test_basic_module via invokeRemoteMethod ─────────────────────────
echo ""
echo "  -- IPC: calls to test_basic_module --"
test_ipc "callBasicEcho(hello)"                   "Result: hello"                          "test_ipc_module.callBasicEcho(hello)"
test_ipc "callBasicEcho(world)"                   "Result: world"                          "test_ipc_module.callBasicEcho(world)"
test_ipc "callBasicAddInts(10, 20)"               "Result: 30"                             "test_ipc_module.callBasicAddInts(10, 20)"
test_ipc "callBasicAddInts(0, 0)"                 "Result: 0"                              "test_ipc_module.callBasicAddInts(0, 0)"
test_ipc "callBasicReturnTrue()"                  "Result: true"                           "test_ipc_module.callBasicReturnTrue()"
test_ipc "callBasicNoArgs()"                      "Result: noArgs()"                       "test_ipc_module.callBasicNoArgs()"
test_ipc "callBasicFiveArgs(a, 1, true, b, 2)"   "Result: fiveArgs(a, 1, true, b, 2)"    "test_ipc_module.callBasicFiveArgs(a, 1, true, b, 2)"
test_ipc "callBasicSuccessResult()"               "Method call successful"                 "test_ipc_module.callBasicSuccessResult()"
test_ipc "callBasicErrorResult()"                 "Method call successful"                 "test_ipc_module.callBasicErrorResult()"
test_ipc "callBasicResultMapField(name)"          "Result: test"                           "test_ipc_module.callBasicResultMapField(name)"
test_ipc "callBasicResultMapField(count)"         "Result: 42"                             "test_ipc_module.callBasicResultMapField(count)"

# ── Calls to test_extlib_module ───────────────────────────────────────────────
echo ""
echo "  -- IPC: calls to test_extlib_module --"
test_ipc "callExtlibReverse(hello)"               "Result: olleh"                          "test_ipc_module.callExtlibReverse(hello)"
test_ipc "callExtlibReverse(abc)"                 "Result: cba"                            "test_ipc_module.callExtlibReverse(abc)"
test_ipc "callExtlibUppercase(hello)"             "Result: HELLO"                          "test_ipc_module.callExtlibUppercase(hello)"
test_ipc "callExtlibCountChars(hello)"            "Result: 5"                              "test_ipc_module.callExtlibCountChars(hello)"

# ── Cross-module chaining ─────────────────────────────────────────────────────
echo ""
echo "  -- IPC: cross-module chaining --"
test_ipc "chainEchoThenReverse(hello)"            "Result: olleh"                          "test_ipc_module.chainEchoThenReverse(hello)"
test_ipc "chainEchoThenReverse(abcdef)"           "Result: fedcba"                         "test_ipc_module.chainEchoThenReverse(abcdef)"
test_ipc "chainUppercaseThenConcat(foo, bar)"     "Result: FOOBAR"                         "test_ipc_module.chainUppercaseThenConcat(foo, bar)"
test_ipc "chainUppercaseThenConcat(hello, world)" "Result: HELLOWORLD"                     "test_ipc_module.chainUppercaseThenConcat(hello, world)"

# ── Generated type-safe wrappers ──────────────────────────────────────────────
echo ""
echo "  -- IPC: generated wrappers (LogosModules) --"
test_ipc "wrapperBasicEcho(hello)"                "Result: hello"                          "test_ipc_module.wrapperBasicEcho(hello)"
test_ipc "wrapperBasicEcho(test123)"              "Result: test123"                        "test_ipc_module.wrapperBasicEcho(test123)"
test_ipc "wrapperExtlibReverse(hello)"            "Result: olleh"                          "test_ipc_module.wrapperExtlibReverse(hello)"
test_ipc "wrapperExtlibReverse(abc)"              "Result: cba"                            "test_ipc_module.wrapperExtlibReverse(abc)"

# ── Events ────────────────────────────────────────────────────────────────────
echo ""
echo "  -- IPC: events --"
skip_test  "triggerBasicEvent(data)"              "void return → invalid QVariant → logoscore exit 1"


# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 4: Multi-call sequences (test sequential -c chaining)
# ═════════════════════════════════════════════════════════════════════════════

echo ""
echo "-----------------------------------------------------------------"
echo " Multi-call sequences"
echo "-----------------------------------------------------------------"

echo ""
echo "  -- Sequential calls in single logoscore invocation --"
TOTAL=$((TOTAL + 1))
# shellcheck disable=SC2086
printf "        cmd: timeout %s %s %s -m %s -l test_basic_module -c ... -c ... -c ...\n" \
    "$CALL_TIMEOUT" "$LOGOSCORE" "$QUIT_FLAG" "$BASIC_DIR"
# shellcheck disable=SC2086
output=$(timeout "$CALL_TIMEOUT" "$LOGOSCORE" $QUIT_FLAG \
    -m "$BASIC_DIR" -l test_basic_module \
    -c "test_basic_module.returnInt()" \
    -c "test_basic_module.echo(chain_test)" \
    -c "test_basic_module.addInts(10, 20)" \
    2>/dev/null) && rc=0 || rc=$?
if [[ $rc -eq 0 ]] && \
   printf '%s' "$output" | grep -qF "Result: 42" && \
   printf '%s' "$output" | grep -qF "Result: chain_test" && \
   printf '%s' "$output" | grep -qF "Result: 30"; then
    PASS=$((PASS + 1))
    printf "  PASS  basic: sequential 3-call chain\n"
else
    FAIL=$((FAIL + 1))
    printf "  FAIL  basic: sequential 3-call chain (output: %s)\n" "$output"
    FAILURES="${FAILURES}  FAIL  basic: sequential 3-call chain\n"
fi

TOTAL=$((TOTAL + 1))
# shellcheck disable=SC2086
printf "        cmd: timeout %s %s %s -m %s -l test_extlib_module -c ... -c ... -c ...\n" \
    "$CALL_TIMEOUT" "$LOGOSCORE" "$QUIT_FLAG" "$EXTLIB_DIR"
# shellcheck disable=SC2086
output=$(timeout "$CALL_TIMEOUT" "$LOGOSCORE" $QUIT_FLAG \
    -m "$EXTLIB_DIR" -l test_extlib_module \
    -c "test_extlib_module.reverseString(hello)" \
    -c "test_extlib_module.uppercaseString(world)" \
    -c "test_extlib_module.libVersion()" \
    2>/dev/null) && rc=0 || rc=$?
if [[ $rc -eq 0 ]] && \
   printf '%s' "$output" | grep -qF "Result: olleh" && \
   printf '%s' "$output" | grep -qF "Result: WORLD" && \
   printf '%s' "$output" | grep -qF "Result: 1.0.0"; then
    PASS=$((PASS + 1))
    printf "  PASS  extlib: sequential 3-call chain\n"
else
    FAIL=$((FAIL + 1))
    printf "  FAIL  extlib: sequential 3-call chain (output: %s)\n" "$output"
    FAILURES="${FAILURES}  FAIL  extlib: sequential 3-call chain\n"
fi

TOTAL=$((TOTAL + 1))
# shellcheck disable=SC2086
printf "        cmd: timeout %s %s %s -m %s -m %s -m %s -l test_ipc_module -c ... -c ... -c ...\n" \
    "$CALL_TIMEOUT" "$LOGOSCORE" "$QUIT_FLAG" "$BASIC_DIR" "$EXTLIB_DIR" "$IPC_DIR"
# shellcheck disable=SC2086
output=$(timeout "$CALL_TIMEOUT" "$LOGOSCORE" $QUIT_FLAG \
    -m "$BASIC_DIR" -m "$EXTLIB_DIR" -m "$IPC_DIR" \
    -l test_ipc_module \
    -c "test_ipc_module.callBasicEcho(chain)" \
    -c "test_ipc_module.callExtlibReverse(hello)" \
    -c "test_ipc_module.callBasicAddInts(5, 7)" \
    2>/dev/null) && rc=0 || rc=$?
if [[ $rc -eq 0 ]] && \
   printf '%s' "$output" | grep -qF "Result: chain" && \
   printf '%s' "$output" | grep -qF "Result: olleh" && \
   printf '%s' "$output" | grep -qF "Result: 12"; then
    PASS=$((PASS + 1))
    printf "  PASS  ipc: sequential 3-call chain\n"
else
    FAIL=$((FAIL + 1))
    printf "  FAIL  ipc: sequential 3-call chain (output: %s)\n" "$output"
    FAILURES="${FAILURES}  FAIL  ipc: sequential 3-call chain\n"
fi


# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 5: Error cases
# ═════════════════════════════════════════════════════════════════════════════

echo ""
echo "-----------------------------------------------------------------"
echo " Error cases"
echo "-----------------------------------------------------------------"

echo ""
echo "  -- Calling non-existent method --"
assert_call_fails "nonexistent method" \
    -m "$BASIC_DIR" -l test_basic_module -c "test_basic_module.noSuchMethod()"

echo ""
echo "  -- Calling non-existent module --"
assert_call_fails "nonexistent module" \
    -m "$BASIC_DIR" -l no_such_module -c "no_such_module.echo(x)"


# ═════════════════════════════════════════════════════════════════════════════
# Summary
# ═════════════════════════════════════════════════════════════════════════════

echo ""
echo "================================================================="
echo " Results: $PASS passed, $FAIL failed, $SKIP skipped (of $TOTAL run)"
echo "================================================================="

if [[ $FAIL -gt 0 ]]; then
    echo ""
    echo "Failures:"
    printf "%b" "$FAILURES"
    echo ""
    exit 1
fi

echo ""
echo "All tests passed."
exit 0
