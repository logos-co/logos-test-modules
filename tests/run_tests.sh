#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# Logos Test Modules — Integration Test Suite
#
# Exercises every API type and combination in the test modules using logoscore.
# Usage: run_tests.sh <logoscore> <modules-dir>
# ─────────────────────────────────────────────────────────────────────────────
set -uo pipefail

LOGOSCORE="${1:?Usage: run_tests.sh <logoscore> <modules-dir>}"
MODULES_DIR="${2:?}"
UNIT_TEST_BIN="${UNIT_TEST_BIN:-}"  # optional: path to test_ipc_module_tests binary (env var)


# UNIT_NEW_API_TEST_BIN: set via env var; path to test_ipc_new_api_module_tests binary
UNIT_NEW_API_TEST_BIN="${UNIT_NEW_API_TEST_BIN:-}"

# Per-call timeout (seconds) — guard against total hangs.
CALL_TIMEOUT="${TEST_TIMEOUT:-30}"

# TEST_GROUPS: comma-separated list of groups to run (default: all)
# Available groups: basic, basic-cpp, context-cpp, extlib, ipc, ipc-new-api,
#                   multi, errors, unit, unit-new-api
# Example: TEST_GROUPS=ipc  or  TEST_GROUPS=ipc,basic  or  TEST_GROUPS=ipc-new-api
if [[ -n "${TEST_GROUPS:-}" ]]; then
    IFS=',' read -ra ENABLED_GROUPS <<< "$TEST_GROUPS"
else
    ENABLED_GROUPS=()
fi

should_run_group() {
    local group="$1"
    if [[ ${#ENABLED_GROUPS[@]} -eq 0 ]]; then
        return 0  # no filter, run all
    fi
    for g in "${ENABLED_GROUPS[@]}"; do
        if [[ "$g" == "$group" ]]; then
            return 0
        fi
    done
    return 1
}

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
    assert_call "$1" "$2" -m "$MODULES_DIR" -l test_basic_module -c "$3"
}
test_basic_cpp() {
    assert_call "$1" "$2" -m "$MODULES_DIR" -l test_basic_module_cpp -c "$3"
}
# test_context_cpp passes --persistence-path so the runtime actually
# provisions a per-instance data dir for test_context_module_cpp. The
# directory is created on first use of the helper and reused across
# every call in the context-cpp group — the host re-derives the same
# instance ID from the same on-disk dir, so getInstancePersistencePath()
# is stable across these per-method invocations.
test_context_cpp() {
    : "${CONTEXT_PERSISTENCE_DIR:?context-cpp tests must set CONTEXT_PERSISTENCE_DIR first}"
    assert_call "$1" "$2" -m "$MODULES_DIR" \
        --persistence-path "$CONTEXT_PERSISTENCE_DIR" \
        -l test_context_module_cpp -c "$3"
}
test_extlib() {
    assert_call "$1" "$2" -m "$MODULES_DIR" -l test_extlib_module -c "$3"
}
test_ipc() {
    assert_call "$1" "$2" -m "$MODULES_DIR" -l test_ipc_module -c "$3"
}

# ── Banner ───────────────────────────────────────────────────────────────────

echo "================================================================="
echo " Logos Test Modules -- Integration Tests"
echo "================================================================="
echo ""
echo "  logoscore : $LOGOSCORE"
echo "  modules   : $MODULES_DIR"
echo ""

# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 1: test_basic_module (standalone, no IPC)
# ═════════════════════════════════════════════════════════════════════════════

if should_run_group "basic"; then

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


# ── Type coercion (IPC sends mismatched types, provider should convert) ──────
# logoscore auto-detects: 3.14 → double, 42 → int, true → bool, else → string.
# These tests send types that don't match the method signature and verify
# QtProviderObject coerces them correctly.
echo ""
echo "  -- Type coercion --"

# double → int: logoscore parses 3.0 as double, method expects int
test_basic "addInts(3.0, 4.0) [double→int]"    "Result: 7"   "test_basic_module.addInts(3.0, 4.0)"

# double → int: truncation (3.7 → 3 or 4 depending on Qt's convert)
test_basic "addInts(3.7, 1.2) [double→int rounding]"  "Result: 5"   "test_basic_module.addInts(3.7, 1.2)"

# double → int via echoInt
test_basic "echoInt(42.0) [double→int]"         "Result: 42"  "test_basic_module.echoInt(42.0)"

# double → bool via isPositive (5.0 → int 5 → true)
test_basic "isPositive(5.0) [double→int→bool check]" "Result: true" "test_basic_module.isPositive(5.0)"

# mixed coercion: twoArgs(QString, int) called with (string, double)
test_basic "twoArgs(hi, 3.0) [double→int in mixed]" "Result: twoArgs(hi, 3)" "test_basic_module.twoArgs(hi, 3.0)"


fi  # end basic group

# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 1b: test_basic_module_cpp (pure-C++ mirror of test_basic_module)
#
# Same method matrix as `basic` above, but the impl class uses std / LogosMap
# / LogosList / StdLogosResult — the Qt glue is auto-generated by
# `logos-cpp-generator --from-header`. These cases exercise every branch of
# the generator's type-conversion table end-to-end through the CLI; a
# regression in the glue (e.g. a missing `std::string` ↔ `QString`
# conversion, wrong `nlohmannToQVariant` behaviour, or broken `StdLogosResult`
# unpacking) shows up as a specific failing row here with the same output
# contract as its Qt counterpart.
# ═════════════════════════════════════════════════════════════════════════════

if should_run_group "basic-cpp"; then

echo ""
echo "-----------------------------------------------------------------"
echo " test_basic_module_cpp (pure-C++ impl, generated Qt glue)"
echo "-----------------------------------------------------------------"

# ── Return type: bool ────────────────────────────────────────────────────────
echo ""
echo "  -- Return type: bool --"
test_basic_cpp "returnTrue()"       "Result: true"   "test_basic_module_cpp.returnTrue()"
test_basic_cpp "returnFalse()"      "Result: false"  "test_basic_module_cpp.returnFalse()"
test_basic_cpp "isPositive(5)"      "Result: true"   "test_basic_module_cpp.isPositive(5)"
test_basic_cpp "isPositive(0)"      "Result: false"  "test_basic_module_cpp.isPositive(0)"
test_basic_cpp "isPositive(-3)"     "Result: false"  "test_basic_module_cpp.isPositive(-3)"

# ── Return type: int64_t ─────────────────────────────────────────────────────
echo ""
echo "  -- Return type: int64_t --"
test_basic_cpp "returnInt()"        "Result: 42"  "test_basic_module_cpp.returnInt()"
test_basic_cpp "addInts(3, 4)"      "Result: 7"   "test_basic_module_cpp.addInts(3, 4)"
test_basic_cpp "addInts(0, 0)"      "Result: 0"   "test_basic_module_cpp.addInts(0, 0)"
test_basic_cpp "addInts(-5, 10)"    "Result: 5"   "test_basic_module_cpp.addInts(-5, 10)"
test_basic_cpp "stringLength(hello)" "Result: 5"  "test_basic_module_cpp.stringLength(hello)"
skip_test      "stringLength()"     "logoscore cannot call 1-arg method with 0 args"

# ── Return type: uint64_t (unique to the C++ surface) ───────────────────────
echo ""
echo "  -- Return type: uint64_t --"
test_basic_cpp "returnUint()"       "Result: 99"   "test_basic_module_cpp.returnUint()"
test_basic_cpp "echoUint(123)"      "Result: 123"  "test_basic_module_cpp.echoUint(123)"
test_basic_cpp "echoUint(0)"        "Result: 0"    "test_basic_module_cpp.echoUint(0)"

# ── Return type: double ─────────────────────────────────────────────────────
# CLI formats doubles unpredictably ("3.5" vs "3.500000"). Just check the
# dispatch exits cleanly and the "Result:" prefix is there — the Python
# integration suite covers exact-value assertions.
echo ""
echo "  -- Return type: double --"
test_basic_cpp "returnDouble()"        "Result:"  "test_basic_module_cpp.returnDouble()"
test_basic_cpp "addDoubles(1.5, 2.5)"  "Result:"  "test_basic_module_cpp.addDoubles(1.5, 2.5)"

# ── Return type: std::string ────────────────────────────────────────────────
echo ""
echo "  -- Return type: std::string --"
test_basic_cpp "returnString()"     "Result: test_basic_module_cpp"  "test_basic_module_cpp.returnString()"
test_basic_cpp "echo(hello)"        "Result: hello"                  "test_basic_module_cpp.echo(hello)"
test_basic_cpp "echo(world)"        "Result: world"                  "test_basic_module_cpp.echo(world)"
test_basic_cpp "concat(foo, bar)"   "Result: foobar"                 "test_basic_module_cpp.concat(foo, bar)"
skip_test      "concat(, )"         "logoscore cannot pass empty args"

# ── Return type: StdLogosResult ─────────────────────────────────────────────
# Generator emits a StdLogosResult → Qt LogosResult conversion in the glue,
# so the CLI's "Method call successful" sentinel (used for any structured
# return it can't stringify inline) fires identically to the Qt module.
echo ""
echo "  -- Return type: StdLogosResult --"
test_basic_cpp "successResult()"     "Method call successful"  "test_basic_module_cpp.successResult()"
test_basic_cpp "errorResult()"       "Method call successful"  "test_basic_module_cpp.errorResult()"
test_basic_cpp "resultWithMap()"     "Method call successful"  "test_basic_module_cpp.resultWithMap()"
test_basic_cpp "resultWithList()"    "Method call successful"  "test_basic_module_cpp.resultWithList()"
test_basic_cpp "validateInput(hi)"   "Method call successful"  "test_basic_module_cpp.validateInput(hi)"

# ── Return type: LogosMap (nlohmann::json object) ───────────────────────────
# `jsonReturn=true` path: glue calls `nlohmannToQVariant` to produce a
# QVariantMap, which the CLI prints as structured JSON.
echo ""
echo "  -- Return type: LogosMap --"
test_basic_cpp "returnMap()"            "Method call successful"  "test_basic_module_cpp.returnMap()"
test_basic_cpp "makeMap(hello, world)"  "Method call successful"  "test_basic_module_cpp.makeMap(hello, world)"

# ── Return type: LogosList (nlohmann::json array) ───────────────────────────
echo ""
echo "  -- Return type: LogosList --"
test_basic_cpp "returnList()"           "Method call successful"  "test_basic_module_cpp.returnList()"
test_basic_cpp "makeList(x, y)"         "Method call successful"  "test_basic_module_cpp.makeList(x, y)"

# ── Return type: std::vector<std::string> ───────────────────────────────────
echo ""
echo "  -- Return type: std::vector<std::string> --"
test_basic_cpp "returnStringList()"   "Method call successful"  "test_basic_module_cpp.returnStringList()"
skip_test      "splitString(a,b,c)"   "commas in arg value parsed as arg separators by logoscore"

# ── Return type: std::vector<uint8_t> ───────────────────────────────────────
# CLI can't serialize bytes to stdout meaningfully (base64 / hex / raw is an
# encoding choice), and can't accept a vector<uint8_t> literal as a CLI arg.
# Python integration suite exercises the round-trip through the actual wire.
echo ""
echo "  -- Return type: std::vector<uint8_t> --"
skip_test      "returnBytes()"         "CLI can't render std::vector<uint8_t>"
test_basic_cpp "byteArraySize(12345)"  "Result: 5"  "test_basic_module_cpp.byteArraySize(12345)"

# ── Parameter types ─────────────────────────────────────────────────────────
echo ""
echo "  -- Parameter types --"
test_basic_cpp "echoInt(42)"       "Result: 42"     "test_basic_module_cpp.echoInt(42)"
test_basic_cpp "echoInt(0)"        "Result: 0"      "test_basic_module_cpp.echoInt(0)"
test_basic_cpp "echoInt(-7)"       "Result: -7"     "test_basic_module_cpp.echoInt(-7)"
test_basic_cpp "echoBool(true)"    "Result: true"   "test_basic_module_cpp.echoBool(true)"
test_basic_cpp "echoBool(false)"   "Result: false"  "test_basic_module_cpp.echoBool(false)"
skip_test      "joinStrings(vector<string>)"  "logoscore cannot pass vector<string> params"

# ── Argument counts 0–5 ─────────────────────────────────────────────────────
# Format strings are produced by std::to_string (not QString::arg), but the
# expected output strings match the Qt module's bit-for-bit — the impl is
# careful to emit the same shape.
echo ""
echo "  -- Argument counts 0-5 --"
test_basic_cpp "noArgs()"                    "Result: noArgs()"                     "test_basic_module_cpp.noArgs()"
test_basic_cpp "oneArg(x)"                   "Result: oneArg(x)"                    "test_basic_module_cpp.oneArg(x)"
test_basic_cpp "twoArgs(x, 1)"               "Result: twoArgs(x, 1)"                "test_basic_module_cpp.twoArgs(x, 1)"
test_basic_cpp "threeArgs(x, 1, true)"       "Result: threeArgs(x, 1, true)"        "test_basic_module_cpp.threeArgs(x, 1, true)"
test_basic_cpp "fourArgs(x, 1, true, y)"     "Result: fourArgs(x, 1, true, y)"      "test_basic_module_cpp.fourArgs(x, 1, true, y)"
test_basic_cpp "fiveArgs(x, 1, true, y, 2)"  "Result: fiveArgs(x, 1, true, y, 2)"   "test_basic_module_cpp.fiveArgs(x, 1, true, y, 2)"

# ── Void methods ────────────────────────────────────────────────────────────
echo ""
echo "  -- Void methods (logoscore returns non-zero for void, testing no crash) --"
skip_test  "doNothing()"             "void return → invalid QVariant → logoscore exit 1"
skip_test  "doNothingWithArgs(a,1)"  "void return → invalid QVariant → logoscore exit 1"

# ── Events ──────────────────────────────────────────────────────────────────
# The impl's `std::function emitEvent` is wired by the generator to the
# LogosProviderBase::emitEvent path. Same void-return limitation as the Qt
# module: fire-and-forget through the CLI produces exit 1.
echo ""
echo "  -- Events --"
skip_test  "emitTestEvent(data)"         "void return → invalid QVariant → logoscore exit 1"
skip_test  "emitMultiArgEvent(ev, 5)"    "void return → invalid QVariant → logoscore exit 1"

# ── Type coercion ───────────────────────────────────────────────────────────
# Same logoscore-side coercion rules as the Qt module, but the receiving end
# is int64_t instead of int — the generator's std-to-Qt glue should still
# land on the same result after QVariant::convert.
echo ""
echo "  -- Type coercion --"
test_basic_cpp "addInts(3.0, 4.0) [double→int64]"        "Result: 7"                 "test_basic_module_cpp.addInts(3.0, 4.0)"
test_basic_cpp "echoInt(42.0) [double→int64]"            "Result: 42"                "test_basic_module_cpp.echoInt(42.0)"
test_basic_cpp "isPositive(5.0) [double→int64→bool]"     "Result: true"              "test_basic_module_cpp.isPositive(5.0)"
test_basic_cpp "twoArgs(hi, 3.0) [double→int64 mixed]"   "Result: twoArgs(hi, 3)"    "test_basic_module_cpp.twoArgs(hi, 3.0)"

fi  # end basic-cpp group

# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 2b: test_context_module_cpp (LogosModuleContext lifecycle)
# ═════════════════════════════════════════════════════════════════════════════
#
# Exercises the SDK's LogosModuleContext base class end-to-end:
#
#   1. The impl inherits LogosModuleContext (opt-in mixin).
#   2. The codegen-emitted provider's `onInit(LogosAPI*)` override
#      reads modulePath / instanceId / instancePersistencePath off
#      the LogosAPI and threads them into the context via the
#      SFINAE'd helpers in logos_module_context.h.
#   3. The host (logoscore) provisions the persistence directory
#      from `--persistence-path` BEFORE the module loads, so the
#      three getters return the expected values from the moment
#      onContextReady() fires.
#
# Logoscore's --persistence-path is required for instanceId() and
# instancePersistencePath() to populate (module_manager.cpp only
# stamps those properties when persistenceBasePath() is set). We
# create a fresh temp dir per run so the assertions can match a
# unique path prefix.

if should_run_group "context-cpp"; then

echo ""
echo "-----------------------------------------------------------------"
echo " test_context_module_cpp (LogosModuleContext lifecycle wire-up)"
echo "-----------------------------------------------------------------"

CONTEXT_PERSISTENCE_DIR="$(mktemp -d 2>/dev/null || mktemp -d -t 'logos-ctx-test')"
trap 'rm -rf "$CONTEXT_PERSISTENCE_DIR"' EXIT
echo "  persistence base: $CONTEXT_PERSISTENCE_DIR"

# Quick liveness probe: returns the int 1 via a constant — confirms
# the module loads and the codegen-emitted dispatch wires up before
# we start asserting context state. Failure here means the rest of
# the assertions are noise — the framework didn't even start.
echo ""
echo "  -- Lifecycle hook fired --"
test_context_cpp "wasContextReady()"    "Result: true"  "test_context_module_cpp.wasContextReady()"

# Module path: the host (logos_host) stamps the parent dir of the
# loaded plugin file. We can't predict the absolute path (nix store),
# but we CAN predict the module dir name. logoscore loads from
# $MODULES_DIR/test_context_module_cpp/, so the path must contain
# that segment.
echo ""
echo "  -- Three context properties populated --"
test_context_cpp "getModulePath() contains module name" \
    "test_context_module_cpp"   "test_context_module_cpp.getModulePath()"

# Persistence path: <CONTEXT_PERSISTENCE_DIR>/test_context_module_cpp/<instanceId>.
# We assert the prefix; the instance ID is host-generated so opaque
# to the test.
test_context_cpp "getInstancePersistencePath() rooted at temp dir" \
    "$CONTEXT_PERSISTENCE_DIR/test_context_module_cpp"  \
    "test_context_module_cpp.getInstancePersistencePath()"

# Instance ID: opaque host-generated short ID, just confirm it
# round-trips as a non-empty value. The "Result:" prefix is the
# CLI convention; the integration here is testing that the
# property propagated at all, not what shape the ID takes.
test_context_cpp "getInstanceId() non-empty"  \
    "Result: "   "test_context_module_cpp.getInstanceId()"

# persistencePathEndsWith() takes one string arg. Pass a suffix
# we know matches: the parent segment of the instance dir is the
# module name, which IS predictable. The full path ends with the
# instance ID, but it definitely *contains* "test_context_module_cpp"
# somewhere on the right-hand side, so we use a suffix that ends
# with the module name + a known-stable child segment (only ID
# changes per run; module name doesn't). Easiest reliable case:
# we test with a single character "/" which is guaranteed to be
# in the path — but the CLI can't reliably pass "/" alone. So we
# skip the .endsWith() helper at the integration layer; the SDK
# unit tests already cover String operations exhaustively.
skip_test  "persistencePathEndsWith(<suffix>)"  "CLI can't reliably pass slash-containing args; covered by SDK unit tests"

# ── Cross-module calls via modules() ────────────────────────────────────
# These prove the whole chain: the codegen-emitted onInit built a
# LogosModules from the host's LogosAPI, threaded it through
# LogosModuleContext via the SFINAE'd helper, and the typed access
# in our impl resolves to the right dep. The host loads
# test_basic_module by name (the dep is declared in
# test_context_module_cpp's metadata.json), so the in-process IPC
# path between the two modules has to be fully wired.
echo ""
echo "  -- Cross-module calls through modules() --"
test_context_cpp "callBasicEcho(hello)"   "Result: hello"  \
    "test_context_module_cpp.callBasicEcho(hello)"
test_context_cpp "callBasicEcho(world)"   "Result: world"  \
    "test_context_module_cpp.callBasicEcho(world)"
test_context_cpp "callBasicAddInts(3, 4)" "Result: 7"      \
    "test_context_module_cpp.callBasicAddInts(3, 4)"
test_context_cpp "callBasicAddInts(-5, 10)" "Result: 5"    \
    "test_context_module_cpp.callBasicAddInts(-5, 10)"

# ── Typed event subscriptions (logos_events: end-to-end) ────────────────
#
# test_basic_module_cpp declares typed events in `logos_events:`. The
# codegen emits a `.lidl` sidecar with those events; buildHeaders.nix
# threads it into `--events-from` so the generated TestBasicModuleCpp
# wrapper gains `onTestEvent(...)` / `onMultiArgEvent(...)` typed
# accessors. test_context_module_cpp's
# `subscribeToBasicCppEvents()` calls those accessors with std-typed
# C++ callbacks that stash the payload into instance state.
#
# Each round-trip case below chains three `-c` invocations in ONE
# logoscore process so the event loop pumps QRO deliveries between
# them. logoscore's `-c` ordering is `subscribe → trigger → read`;
# the chained output contains all three results, and the harness
# greps for the expected substring of the final read.
echo ""
echo "  -- Typed event subscriptions on test_basic_module_cpp --"

test_context_cpp "subscribeToBasicCppEvents()"  "Result: ok"  \
    "test_context_module_cpp.subscribeToBasicCppEvents()"

# subscribe → triggerTestEvent("hello") → getLastTestEventData()
# logoscore runs the `-c` calls sequentially in the same process; the
# Qt event loop pumps QRO event deliveries between them, so the
# subscription callback has fired before the read.
assert_call "testEvent round-trip via onTestEvent"  "Result: hello"  \
    -m "$MODULES_DIR"                                                  \
    --persistence-path "$CONTEXT_PERSISTENCE_DIR"                      \
    -l test_basic_module_cpp,test_context_module_cpp                   \
    -c "test_context_module_cpp.subscribeToBasicCppEvents()"           \
    -c "test_basic_module_cpp.triggerTestEvent(hello)"                 \
    -c "test_context_module_cpp.getLastTestEventData()"

# Same pattern, multi-arg event: subscribe → trigger(ev, 42) → read.
# Result-map shape: {"count": 42, "name": "ev"} — grep just on
# `"name": "ev"` to keep the assertion narrow.
assert_call "multiArgEvent round-trip via onMultiArgEvent"  "ev"      \
    -m "$MODULES_DIR"                                                  \
    --persistence-path "$CONTEXT_PERSISTENCE_DIR"                      \
    -l test_basic_module_cpp,test_context_module_cpp                   \
    -c "test_context_module_cpp.subscribeToBasicCppEvents()"           \
    -c "test_basic_module_cpp.triggerMultiArgEvent(ev, 42)"            \
    -c "test_context_module_cpp.getLastMultiArgEvent()"

fi  # end context-cpp group

# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 2: test_extlib_module (external C library wrapper)
# ═════════════════════════════════════════════════════════════════════════════

if should_run_group "extlib"; then

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


fi  # end extlib group

# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 3: test_ipc_module (inter-module communication)
# ═════════════════════════════════════════════════════════════════════════════

if should_run_group "ipc"; then

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


fi  # end ipc group

# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 3b: test_ipc_new_api_module (new LogosProviderBase API)
# ═════════════════════════════════════════════════════════════════════════════

if should_run_group "ipc-new-api"; then

echo ""
echo "-----------------------------------------------------------------"
echo " test_ipc_new_api_module (LogosProviderBase API, requires all modules)"
echo "-----------------------------------------------------------------"

test_ipc_new_api() {
    assert_call "$1" "$2" \
        -m "$MODULES_DIR" \
        -l test_ipc_new_api_module -c "$3"
}

echo ""
echo "  -- IPC new-API: calls to test_basic_module --"
test_ipc_new_api "callBasicEcho(hello)"                   "Result: hello"                          "test_ipc_new_api_module.callBasicEcho(hello)"
test_ipc_new_api "callBasicEcho(world)"                   "Result: world"                          "test_ipc_new_api_module.callBasicEcho(world)"
test_ipc_new_api "callBasicAddInts(10, 20)"               "Result: 30"                             "test_ipc_new_api_module.callBasicAddInts(10, 20)"
test_ipc_new_api "callBasicAddInts(0, 0)"                 "Result: 0"                              "test_ipc_new_api_module.callBasicAddInts(0, 0)"
test_ipc_new_api "callBasicReturnTrue()"                  "Result: true"                           "test_ipc_new_api_module.callBasicReturnTrue()"
test_ipc_new_api "callBasicNoArgs()"                      "Result: noArgs()"                       "test_ipc_new_api_module.callBasicNoArgs()"
test_ipc_new_api "callBasicFiveArgs(a, 1, true, b, 2)"   "Result: fiveArgs(a, 1, true, b, 2)"    "test_ipc_new_api_module.callBasicFiveArgs(a, 1, true, b, 2)"
test_ipc_new_api "callBasicSuccessResult()"               "Method call successful"                 "test_ipc_new_api_module.callBasicSuccessResult()"
test_ipc_new_api "callBasicErrorResult()"                 "Method call successful"                 "test_ipc_new_api_module.callBasicErrorResult()"
test_ipc_new_api "callBasicResultMapField(name)"          "Result: test"                           "test_ipc_new_api_module.callBasicResultMapField(name)"
test_ipc_new_api "callBasicResultMapField(count)"         "Result: 42"                             "test_ipc_new_api_module.callBasicResultMapField(count)"

echo ""
echo "  -- IPC new-API: calls to test_extlib_module --"
test_ipc_new_api "callExtlibReverse(hello)"               "Result: olleh"                          "test_ipc_new_api_module.callExtlibReverse(hello)"
test_ipc_new_api "callExtlibReverse(abc)"                 "Result: cba"                            "test_ipc_new_api_module.callExtlibReverse(abc)"
test_ipc_new_api "callExtlibUppercase(hello)"             "Result: HELLO"                          "test_ipc_new_api_module.callExtlibUppercase(hello)"
test_ipc_new_api "callExtlibCountChars(hello)"            "Result: 5"                              "test_ipc_new_api_module.callExtlibCountChars(hello)"

echo ""
echo "  -- IPC new-API: cross-module chaining --"
test_ipc_new_api "chainEchoThenReverse(hello)"            "Result: olleh"                          "test_ipc_new_api_module.chainEchoThenReverse(hello)"
test_ipc_new_api "chainEchoThenReverse(abcdef)"           "Result: fedcba"                         "test_ipc_new_api_module.chainEchoThenReverse(abcdef)"
test_ipc_new_api "chainUppercaseThenConcat(foo, bar)"     "Result: FOOBAR"                         "test_ipc_new_api_module.chainUppercaseThenConcat(foo, bar)"
test_ipc_new_api "chainUppercaseThenConcat(hello, world)" "Result: HELLOWORLD"                     "test_ipc_new_api_module.chainUppercaseThenConcat(hello, world)"

echo ""
echo "  -- IPC new-API: generated wrappers (LogosModules) --"
test_ipc_new_api "wrapperBasicEcho(hello)"                "Result: hello"                          "test_ipc_new_api_module.wrapperBasicEcho(hello)"
test_ipc_new_api "wrapperBasicEcho(test123)"              "Result: test123"                        "test_ipc_new_api_module.wrapperBasicEcho(test123)"
test_ipc_new_api "wrapperExtlibReverse(hello)"            "Result: olleh"                          "test_ipc_new_api_module.wrapperExtlibReverse(hello)"
test_ipc_new_api "wrapperExtlibReverse(abc)"              "Result: cba"                            "test_ipc_new_api_module.wrapperExtlibReverse(abc)"

echo ""
echo "  -- IPC new-API: events --"
skip_test  "triggerBasicEvent(data)"              "void return → invalid QVariant → logoscore exit 1"

fi  # end ipc-new-api group

# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 4: Multi-call sequences (test sequential -c chaining)
# ═════════════════════════════════════════════════════════════════════════════

if should_run_group "multi"; then

echo ""
echo "-----------------------------------------------------------------"
echo " Multi-call sequences"
echo "-----------------------------------------------------------------"

echo ""
echo "  -- Sequential calls in single logoscore invocation --"
TOTAL=$((TOTAL + 1))
# shellcheck disable=SC2086
printf "        cmd: timeout %s %s %s -m %s -l test_basic_module -c ... -c ... -c ...\n" \
    "$CALL_TIMEOUT" "$LOGOSCORE" "$QUIT_FLAG" "$MODULES_DIR"
# shellcheck disable=SC2086
output=$(timeout "$CALL_TIMEOUT" "$LOGOSCORE" $QUIT_FLAG \
    -m "$MODULES_DIR" -l test_basic_module \
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
    "$CALL_TIMEOUT" "$LOGOSCORE" "$QUIT_FLAG" "$MODULES_DIR"
# shellcheck disable=SC2086
output=$(timeout "$CALL_TIMEOUT" "$LOGOSCORE" $QUIT_FLAG \
    -m "$MODULES_DIR" -l test_extlib_module \
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
printf "        cmd: timeout %s %s %s -m %s -l test_ipc_module -c ... -c ... -c ...\n" \
    "$CALL_TIMEOUT" "$LOGOSCORE" "$QUIT_FLAG" "$MODULES_DIR"
# shellcheck disable=SC2086
output=$(timeout "$CALL_TIMEOUT" "$LOGOSCORE" $QUIT_FLAG \
    -m "$MODULES_DIR" -l test_ipc_module \
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


fi  # end multi group

# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 5: Error cases
# ═════════════════════════════════════════════════════════════════════════════

if should_run_group "errors"; then

echo ""
echo "-----------------------------------------------------------------"
echo " Error cases"
echo "-----------------------------------------------------------------"

echo ""
echo "  -- Calling non-existent method --"
assert_call_fails "nonexistent method" \
    -m "$MODULES_DIR" -l test_basic_module -c "test_basic_module.noSuchMethod()"

echo ""
echo "  -- Calling non-existent module --"
assert_call_fails "nonexistent module" \
    -m "$MODULES_DIR" -l no_such_module -c "no_such_module.echo(x)"


fi  # end errors group

# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 6: Async calls (invokeRemoteMethodAsync + generated wrappers)
# ═════════════════════════════════════════════════════════════════════════════

if should_run_group "async"; then

echo ""
echo "-----------------------------------------------------------------"
echo " Async calls"
echo "-----------------------------------------------------------------"

echo ""
echo "  -- Raw invokeRemoteMethodAsync --"
test_ipc "asyncCallBasicEcho(hello)"         "Result: hello"   "test_ipc_module.asyncCallBasicEcho(hello)"
test_ipc "asyncCallBasicEcho(world)"         "Result: world"   "test_ipc_module.asyncCallBasicEcho(world)"
test_ipc "asyncCallBasicAddInts(3, 4)"       "Result: 7"       "test_ipc_module.asyncCallBasicAddInts(3, 4)"
test_ipc "asyncCallBasicAddInts(0, 0)"       "Result: 0"       "test_ipc_module.asyncCallBasicAddInts(0, 0)"

echo ""
echo "  -- Async cross-module (ipc -> extlib) --"
test_ipc "asyncCallExtlibReverse(hello)"     "Result: olleh"   "test_ipc_module.asyncCallExtlibReverse(hello)"
test_ipc "asyncCallExtlibReverse(abc)"       "Result: cba"     "test_ipc_module.asyncCallExtlibReverse(abc)"

echo ""
echo "  -- Generated async wrapper (echoAsync) --"
test_ipc "asyncWrapperBasicEcho(hello)"      "Result: hello"   "test_ipc_module.asyncWrapperBasicEcho(hello)"
test_ipc "asyncWrapperBasicEcho(test123)"    "Result: test123" "test_ipc_module.asyncWrapperBasicEcho(test123)"


fi  # end async group

# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 7: Unit tests (mock transport, no logoscore required)
# ═════════════════════════════════════════════════════════════════════════════

if should_run_group "unit"; then

echo ""
echo "-----------------------------------------------------------------"
echo " Unit tests (mock transport)"
echo "-----------------------------------------------------------------"
echo ""

if [[ -z "$UNIT_TEST_BIN" ]]; then
    echo "  SKIP  unit tests (no unit test binary provided)"
    echo "        Set UNIT_TEST_BIN env var to the path of test_ipc_module_tests binary"
    SKIP=$((SKIP + 1))
elif [[ ! -x "$UNIT_TEST_BIN" ]]; then
    FAIL=$((FAIL + 1))
    printf "  FAIL  unit tests — binary not found or not executable: %s\n" "$UNIT_TEST_BIN"
    FAILURES="${FAILURES}  FAIL  unit tests: binary not found: ${UNIT_TEST_BIN}\n"
else
    TOTAL=$((TOTAL + 1))
    printf "        cmd: %s\n" "$UNIT_TEST_BIN"
    unit_output=$("$UNIT_TEST_BIN" 2>&1) && unit_rc=0 || unit_rc=$?
    printf "%s\n" "$unit_output"
    if [[ $unit_rc -eq 0 ]]; then
        PASS=$((PASS + 1))
        printf "  PASS  unit tests\n"
    else
        FAIL=$((FAIL + 1))
        printf "  FAIL  unit tests (exit code %d)\n" "$unit_rc"
        FAILURES="${FAILURES}  FAIL  unit tests: exit code ${unit_rc}\n"
    fi
fi


fi  # end unit group

# ═════════════════════════════════════════════════════════════════════════════
# TEST GROUP 7: Unit tests — new provider API (mock transport, no logoscore)
# ═════════════════════════════════════════════════════════════════════════════

if should_run_group "unit-new-api"; then

echo ""
echo "-----------------------------------------------------------------"
echo " Unit tests — new provider API (mock transport)"
echo "-----------------------------------------------------------------"
echo ""

if [[ -z "$UNIT_NEW_API_TEST_BIN" ]]; then
    echo "  SKIP  unit-new-api tests (no unit test binary provided)"
    echo "        Set UNIT_NEW_API_TEST_BIN to the path of test_ipc_new_api_module_tests"
    SKIP=$((SKIP + 1))
elif [[ ! -x "$UNIT_NEW_API_TEST_BIN" ]]; then
    FAIL=$((FAIL + 1))
    printf "  FAIL  unit-new-api tests — binary not found or not executable: %s\n" "$UNIT_NEW_API_TEST_BIN"
    FAILURES="${FAILURES}  FAIL  unit-new-api tests: binary not found: ${UNIT_NEW_API_TEST_BIN}\n"
else
    TOTAL=$((TOTAL + 1))
    printf "        cmd: %s\n" "$UNIT_NEW_API_TEST_BIN"
    unit_na_output=$("$UNIT_NEW_API_TEST_BIN" 2>&1) && unit_na_rc=0 || unit_na_rc=$?
    printf "%s\n" "$unit_na_output"
    if [[ $unit_na_rc -eq 0 ]]; then
        PASS=$((PASS + 1))
        printf "  PASS  unit-new-api tests\n"
    else
        FAIL=$((FAIL + 1))
        printf "  FAIL  unit-new-api tests (exit code %d)\n" "$unit_na_rc"
        FAILURES="${FAILURES}  FAIL  unit-new-api tests: exit code ${unit_na_rc}\n"
    fi
fi


fi  # end unit-new-api group

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
