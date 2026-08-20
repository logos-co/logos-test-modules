#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# The module teardown contract — LogosModuleContext::aboutToUnload()
#
# Usage: run_unload_tests.sh <logoscore> <modules-dir>
#
# A module gets one chance to finish work before it is torn down. It answers
# Synchronous ("already quiescent") or Asynchronous ("wait for me") and then
# calls unloadFinished(). The host waits — for a BOUNDED grace period.
#
# Three daemon lifecycles, one per mode, because the thing under test is what
# happens during SHUTDOWN; a single long-lived daemon cannot exercise it.
#
# WHY THE ASSERTIONS ARE ABOUT JOURNAL LINES AND NOT ELAPSED TIME. A timing
# threshold ("teardown took >1.5s, so the host waited") is exactly the shape
# that flakes on a loaded CI box, and it fails in the direction that hides
# regressions. The journal gives the same proof structurally:
#
#   async: FINISHED is written 1.5s into teardown. A host that did NOT wait
#          kills the module in well under a second, so the line cannot be
#          there unless the host waited. Its PRESENCE is the proof.
#
#   hang:  the module asks to wait and never finishes. FINISHED must be
#          ABSENT, and the daemon must still exit. Its absence plus a clean
#          exit is the proof the deadline is enforced rather than merely
#          configured.
#
# Elapsed time is printed for a human reading the log, and deliberately never
# asserted on.
# ─────────────────────────────────────────────────────────────────────────────
set -uo pipefail

LOGOSCORE="${1:?Usage: run_unload_tests.sh <logoscore> <modules-dir>}"
MODULES_DIR="${2:?}"

MODULE="test_unload_module_cpp"
# Generous: this bounds a HANG, so it must exceed the host's grace period by a
# wide margin. Tripping it means the daemon never exited, which is the failure
# this suite exists to catch — not a slow machine.
DAEMON_EXIT_TIMEOUT="${UNLOAD_DAEMON_TIMEOUT:-60}"

PASS=0
FAIL=0

ok()   { echo "  PASS  $1"; PASS=$((PASS+1)); }
bad()  { echo "  FAIL  $1"; echo "        $2"; FAIL=$((FAIL+1)); }

# Runs one daemon lifecycle in `mode` and echoes the journal it produced.
run_case() {
    local mode="$1" workdir="$2"
    rm -rf "$workdir"; mkdir -p "$workdir/config"
    local journal="$workdir/journal.txt"

    # Per-case config dir: a leaked daemon from an earlier case would otherwise
    # answer for this one, and the failure looks like a teardown bug.
    LOGOSCORE_CONFIG_DIR="$workdir/config" \
    LOGOS_UNLOAD_MODE="$mode" \
    LOGOS_UNLOAD_JOURNAL="$journal" \
        "$LOGOSCORE" -D -m "$MODULES_DIR" > "$workdir/daemon.log" 2>&1 &
    local dpid=$!

    # Wait for readiness rather than sleeping a fixed amount.
    local waited=0
    until LOGOSCORE_CONFIG_DIR="$workdir/config" "$LOGOSCORE" status >/dev/null 2>&1; do
        sleep 0.5; waited=$((waited+1))
        if [ "$waited" -gt 60 ]; then
            echo "DAEMON_NEVER_READY"; kill -9 "$dpid" 2>/dev/null; return 1
        fi
    done

    # A module that never loaded has nothing to tear down, and an empty journal
    # then reads exactly like a teardown hook that never fired. Say which it is.
    if ! LOGOSCORE_CONFIG_DIR="$workdir/config" \
            "$LOGOSCORE" load-module "$MODULE" >"$workdir/load.json" 2>&1; then
        echo "MODULE_NEVER_LOADED: $(tr -d '\n' < "$workdir/load.json")"
        LOGOSCORE_CONFIG_DIR="$workdir/config" "$LOGOSCORE" stop >/dev/null 2>&1
        kill -9 "$dpid" 2>/dev/null
        return 1
    fi

    local start; start=$(date +%s)
    LOGOSCORE_CONFIG_DIR="$workdir/config" "$LOGOSCORE" stop >/dev/null 2>&1

    # Bounded wait for the daemon process itself to go away. `stop` returns as
    # soon as the daemon acknowledges; teardown happens after that, and teardown
    # is the thing being measured.
    local spent=0
    while kill -0 "$dpid" 2>/dev/null; do
        sleep 0.5; spent=$((spent+1))
        if [ "$spent" -gt $((DAEMON_EXIT_TIMEOUT * 2)) ]; then
            echo "DAEMON_NEVER_EXITED"; kill -9 "$dpid" 2>/dev/null; return 1
        fi
    done
    wait "$dpid" 2>/dev/null
    local end; end=$(date +%s)

    echo "    [$mode] daemon teardown ~$((end-start))s" >&2
    tr '\n' ' ' < "$journal" 2>/dev/null
    echo
}

echo "═══ module teardown contract ═══"
WORK="${TMPDIR:-/tmp}/logos-unload-tests.$$"

# ── sync ────────────────────────────────────────────────────────────────────
# The hook is reached at all. Without this, the two cases below could both pass
# for the wrong reason (a module whose hook never fires also never writes
# FINISHED).
j=$(run_case sync "$WORK/sync")
case "$j" in
  DAEMON_NEVER_*) bad "sync: daemon lifecycle" "$j" ;;
  MODULE_NEVER_LOADED*) bad "sync: module loads" "$j" ;;
  *ENTERED*)      ok  "sync: aboutToUnload() is reached" ;;
  *)              bad "sync: aboutToUnload() is reached" \
                      "journal was: '$j'. An EMPTY journal here almost always means the
        HOST does not call the hook, not that the module is wrong: the wait lives in
        logos-module-loader-qt's logos_host, so a logoscore whose liblogos predates it
        tears modules down without ever asking. Check that pin before the module." ;;
esac

# ── async, finishes ─────────────────────────────────────────────────────────
j=$(run_case async "$WORK/async")
case "$j" in
  DAEMON_NEVER_*) bad "async: daemon lifecycle" "$j" ;;
  *ENTERED*FINISHED*)
      ok "async: the host WAITED for the module to finish" ;;
  *ENTERED*)
      bad "async: the host WAITED for the module to finish" \
          "the module was torn down before it finished; journal was: '$j'" ;;
  *)  bad "async: aboutToUnload() is reached" "journal was: '$j'" ;;
esac

# ── async, never finishes ───────────────────────────────────────────────────
j=$(run_case hang "$WORK/hang")
case "$j" in
  DAEMON_NEVER_EXITED)
      bad "hang: the deadline is enforced" \
          "a module that never finishes prevented the daemon from exiting" ;;
  DAEMON_NEVER_*) bad "hang: daemon lifecycle" "$j" ;;
  *FINISHED*)
      bad "hang: the module must NOT report finishing" \
          "journal was: '$j' — the fixture, not the host, is wrong" ;;
  *ENTERED*)
      ok "hang: the host gave up on a module that never finished" ;;
  *)  bad "hang: aboutToUnload() is reached" "journal was: '$j'" ;;
esac

rm -rf "$WORK"
echo "═══ $PASS passed, $FAIL failed ═══"
[ "$FAIL" -eq 0 ]
