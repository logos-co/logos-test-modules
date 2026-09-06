#!/usr/bin/env bash
#
# End-to-end assertion for `metadata.json#optional_dependencies`.
#
# test_optional_module_cpp declares test_basic_module_cpp as OPTIONAL and
# declares nothing as required. That is the whole setup, and it is what makes
# the properties observable across TWO runs against TWO different modules
# directories:
#
#   ALONE     — only the consumer is on disk. It must load, and it must report
#               the dependency as absent rather than hanging on it.
#   WITH DEP  — both are on disk. The same typed call must now succeed, which
#               is what proves the wrapper was real and not a stub.
#
# Two directories rather than one, because "absent" has to mean absent: staging
# both and merely declining to load one leaves the dependency findable, and
# `load-module` auto-resolves declared dependencies.
set -uo pipefail

LOGOSCORE="${1:?usage: $0 <logoscore> <alone-dir> <with-dep-dir>}"
ALONE_DIR="${2:?missing alone modules dir}"
WITHDEP_DIR="${3:?missing with-dep modules dir}"

FAILURES=0
pass() { echo "  PASS: $1"; }
fail() { echo "  FAIL: $1"; FAILURES=$((FAILURES + 1)); }

command -v jq >/dev/null || { echo "ERROR: jq is required" >&2; exit 1; }

CFG=""
DAEMON_PID=""
stop_daemon() {
  [[ -n "$DAEMON_PID" ]] && kill "$DAEMON_PID" 2>/dev/null
  [[ -n "$CFG" ]] && "$LOGOSCORE" --config-dir "$CFG" stop >/dev/null 2>&1
  [[ -n "$DAEMON_PID" ]] && wait "$DAEMON_PID" 2>/dev/null
  DAEMON_PID=""
}
trap stop_daemon EXIT

# Brings up a daemon over `dir`. Each phase gets its OWN config dir: a leaked
# daemon from a previous phase would serve the previous modules directory and
# the absent-dependency phase would silently find the dependency.
start_daemon() {
  local dir="$1"
  stop_daemon
  CFG="$(mktemp -d 2>/dev/null || mktemp -d -t 'lgc')"
  "$LOGOSCORE" -D --config-dir "$CFG" -m "$dir" >"$CFG/daemon.log" 2>&1 &
  DAEMON_PID=$!
  local i
  for i in $(seq 1 150); do
    "$LOGOSCORE" --config-dir "$CFG" status >/dev/null 2>&1 && return 0
    kill -0 "$DAEMON_PID" 2>/dev/null || break
    sleep 0.2
  done
  echo "ERROR: daemon failed to start over $dir; log:" >&2
  cat "$CFG/daemon.log" >&2 || true
  return 1
}

# Echoes the JSON result value, or the empty string on failure.
call() {
  local mod="$1" method="$2"; shift 2
  local out
  out=$(timeout 60 "$LOGOSCORE" --json --config-dir "$CFG" call "$mod" "$method" "$@" 2>/dev/null)
  [[ "$(printf '%s' "$out" | jq -r '.status // "error"')" == "ok" ]] || return 1
  printf '%s' "$out" | jq -rc '.result'
}

echo "=============================================="
echo " optional_dependencies — end to end"
echo "=============================================="
ls "$ALONE_DIR"   | sed 's/^/  alone:   /'
ls "$WITHDEP_DIR" | sed 's/^/  withdep: /'

# ── 1. Build-time: the dependency is not bundled ──────────────────────────
# A required dependency would have been dragged into the consumer's install
# output. An optional one must not be — that is what keeps a consumer from
# inheriting the dependency's runtime closure.
echo
echo "[1] the optional dependency is NOT bundled with its consumer"
if [ -e "$ALONE_DIR/test_basic_module_cpp" ]; then
  fail "test_basic_module_cpp was bundled into the consumer's install output"
else
  pass "consumer's install output carries no copy of its optional dependency"
fi

# ── 2. Loads with the dependency absent ───────────────────────────────────
echo
echo "[2] the consumer LOADS with its optional dependency absent"
if ! start_daemon "$ALONE_DIR"; then
  fail "daemon would not start over the alone dir"
else
  if "$LOGOSCORE" --config-dir "$CFG" load-module test_optional_module_cpp >/dev/null 2>&1; then
    pass "load-module succeeded with the dependency absent"
  else
    fail "load-module FAILED for a module whose only dependency is optional"
    cat "$CFG/daemon.log" | tail -20 >&2
  fi
  if alive=$(call test_optional_module_cpp selfCheck) && [ "$alive" = "alive" ]; then
    pass "the module is loaded and answering"
  else
    fail "the module did not answer selfCheck() (got: '${alive:-<none>}')"
  fi
  # A required dependency that is missing is a resolution failure and says so.
  if grep -qiE "Missing dependencies|Cannot resolve dependencies" "$CFG/daemon.log"; then
    fail "the loader treated an OPTIONAL dependency as a missing required one"
  else
    pass "the loader did not report a missing dependency"
  fi
  # And it must not have brought the dependency up behind our back.
  if "$LOGOSCORE" --config-dir "$CFG" call test_basic_module_cpp returnTrue >/dev/null 2>&1; then
    fail "the optional dependency was auto-loaded — it must not be"
  else
    pass "the optional dependency was NOT auto-loaded"
  fi

  # ── 3. Absence is reported, bounded by the caller's deadline ────────────
  echo
  echo "[3] a call to the absent dependency fails FAST and says why"
  start=$(date +%s)
  answer=$(call test_optional_module_cpp callEcho hello 1500)
  elapsed=$(( $(date +%s) - start ))
  echo "    | answer: ${answer:-<call failed>}  (${elapsed}s)"
  case "${answer:-}" in
    ABSENT:*) pass "reported a failure class (${answer}) rather than an empty answer" ;;
    *)        fail "expected ABSENT:<code>, got '${answer:-<call failed>}'" ;;
  esac
  # 20s is the protocol default; near it means the caller's deadline was ignored.
  if [ "$elapsed" -lt 15 ]; then
    pass "absence cost ${elapsed}s, not the 20s protocol default"
  else
    fail "the call took ${elapsed}s — the caller's deadline did not bound it"
  fi

fi

# ── 4. The same typed call works once the dependency is there ─────────────
# Without this, everything above would also pass against a wrapper that never
# worked at all.
echo
echo "[4] the SAME typed call succeeds once the dependency is loaded"
if ! start_daemon "$WITHDEP_DIR"; then
  fail "daemon would not start over the with-dep dir"
else
  "$LOGOSCORE" --config-dir "$CFG" load-module test_basic_module_cpp >/dev/null 2>&1 \
    || fail "could not load test_basic_module_cpp"
  "$LOGOSCORE" --config-dir "$CFG" load-module test_optional_module_cpp >/dev/null 2>&1 \
    || fail "could not load test_optional_module_cpp alongside its dependency"

  echoed=$(call test_optional_module_cpp callEcho hello 5000)
  echo "    | answer: ${echoed:-<call failed>}"
  case "${echoed:-}" in
    ABSENT:*) fail "still reported ${echoed} with the dependency loaded" ;;
    hello)    pass "the typed wrapper returned the dependency's real answer" ;;
    *)        fail "expected 'hello', got '${echoed:-<call failed>}'" ;;
  esac

fi

echo
echo "=============================================="
if [ "$FAILURES" -eq 0 ]; then
  echo " optional_dependencies: all checks passed"
  exit 0
fi
echo " optional_dependencies: $FAILURES check(s) FAILED"
exit 1
