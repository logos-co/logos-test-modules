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
LGX_DIR="${4:?missing consumer .lgx dir}"

FAILURES=0
pass() { echo "  PASS: $1"; }
fail() { echo "  FAIL: $1"; FAILURES=$((FAILURES + 1)); }

command -v jq >/dev/null || { echo "ERROR: jq is required" >&2; exit 1; }

CFG=""
DAEMON_PID=""
cleanup() { stop_daemon; rm -f "${ERRFILE:-}"; }
stop_daemon() {
  [[ -n "$DAEMON_PID" ]] && kill "$DAEMON_PID" 2>/dev/null
  [[ -n "$CFG" ]] && "$LOGOSCORE" --config-dir "$CFG" stop >/dev/null 2>&1
  [[ -n "$DAEMON_PID" ]] && wait "$DAEMON_PID" 2>/dev/null
  DAEMON_PID=""
}
trap cleanup EXIT

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

# Echoes the JSON result value on success. On failure it echoes nothing and
# leaves the reason in CALL_ERR: "the call failed" and "the call was refused"
# are different verdicts here, and a runner that cannot tell them apart sends
# you looking for a bug in the feature when the answer is in the daemon log.
# Through a FILE, because every caller here reads the result out of `$(call
# ...)` — a subshell, whose variables do not come back.
ERRFILE="$(mktemp 2>/dev/null || mktemp -t 'lgcerr')"
call() {
  local mod="$1" method="$2" out rc err; shift 2
  out=$(timeout 60 "$LOGOSCORE" --json --config-dir "$CFG" call "$mod" "$method" "$@" 2>&1); rc=$?
  if [[ "$(printf '%s' "$out" | jq -r '.status // "error"' 2>/dev/null)" == "ok" ]]; then
    : >"$ERRFILE"
    printf '%s' "$out" | jq -rc '.result'
    return 0
  fi
  err=$(printf '%s' "$out" | jq -rc 'del(.status) | select(length > 0) | tostring' 2>/dev/null)
  printf '%s' "${err:-exit ${rc}: ${out:-<no output>}}" >"$ERRFILE"
  return 1
}

# What a failed call knows, in the order you want to read it.
diagnose() {
  local err; err=$(cat "$ERRFILE" 2>/dev/null)
  echo "    | error: ${err:-<none reported>}"
  [[ -n "$CFG" && -s "$CFG/daemon.log" ]] && tail -15 "$CFG/daemon.log" | sed 's/^/    | log: /'
  return 0
}

# load-module, keeping what it said. A refused load and an absent plugin look
# identical through a discarded stderr.
load() {
  local out
  out=$("$LOGOSCORE" --config-dir "$CFG" load-module "$1" 2>&1) && return 0
  echo "    | load-module $1: ${out:-<no output>}"
  return 1
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
  if load test_optional_module_cpp; then
    pass "load-module succeeded with the dependency absent"
  else
    fail "load-module FAILED for a module whose only dependency is optional"
    diagnose
  fi
  if alive=$(call test_optional_module_cpp selfCheck) && [ "$alive" = "alive" ]; then
    pass "the module is loaded and answering"
  else
    fail "the module did not answer selfCheck() (got: '${alive:-<none>}')"
    diagnose
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
    *)        fail "expected ABSENT:<code>, got '${answer:-<call failed>}'"; diagnose ;;
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
  load test_basic_module_cpp || fail "could not load test_basic_module_cpp"
  load test_optional_module_cpp \
    || fail "could not load test_optional_module_cpp alongside its dependency"

  echoed=$(call test_optional_module_cpp callEcho hello 5000)
  echo "    | answer: ${echoed:-<call failed>}"
  case "${echoed:-}" in
    ABSENT:*) fail "still reported ${echoed} with the dependency loaded" ;;
    hello)    pass "the typed wrapper returned the dependency's real answer" ;;
    *)        fail "expected 'hello', got '${echoed:-<call failed>}'"; diagnose ;;
  esac

fi

# ── 5. The declaration survives packaging ────────────────────────────────
# metadata.json is the source, but the .lgx `manifest.json` is the only copy an
# installer or catalog reads BEFORE unpacking. The distinction has to survive
# the trip, or an absent optional dependency reads as a broken install.
echo
echo "[5] the .lgx manifest keeps the dependency OPTIONAL"
manifest=$(tar -xOzf "$LGX_DIR"/*.lgx manifest.json 2>/dev/null)
if [ -z "$manifest" ]; then
  fail "could not read manifest.json out of $LGX_DIR"
else
  ver=$(printf '%s' "$manifest" | jq -r '.manifestVersion // "<none>"')
  opt=$(printf '%s' "$manifest" | jq -rc '.optional_dependencies // empty')
  req=$(printf '%s' "$manifest" | jq -rc '.dependencies // empty')
  echo "    | manifestVersion $ver — dependencies: ${req:-<absent>}, optional: ${opt:-<absent>}"
  # Not the version: that bumps on its own schedule, and the key being there at
  # all is what a stale bundler pin would lose.
  if [ "$opt" = '["test_basic_module_cpp"]' ]; then
    pass "the manifest carries optional_dependencies"
  else
    fail "expected optional_dependencies ['test_basic_module_cpp'], got '${opt:-<absent>}'"
  fi
  case "${req:-}" in
    *test_basic_module_cpp*)
      fail "the optional dependency also appears in the manifest's REQUIRED list" ;;
    *)
      pass "it is not in the required list, so an absent one is not a broken install" ;;
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
