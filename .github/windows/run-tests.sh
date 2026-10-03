#!/usr/bin/env bash
# The tests/*.sh suites against the staged logoscore.exe and the portable
# fixture modules (packages.x86_64-windows). Run by windows.yml from the stage root.
set -euo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
# POSIX spellings: GNU tar reads "D:" as a remote host; bash converts them for logoscore.exe.
stage="$(cygpath -u "$STAGE_ABS")"
logoscore="$stage/cli/bin/logoscore.exe"
fixtures="$stage/fixtures"

rc=0
bash "$repo/tests/run_tests.sh" "$logoscore" "$fixtures/modules" || rc=1
for lang in cpp rust; do
  bash "$repo/tests/run_unload_tests.sh" "$logoscore" "$fixtures/unload-$lang" "test_unload_module_$lang" || rc=1
done
bash "$repo/tests/run_optional_dependency_tests.sh" "$logoscore" \
  "$fixtures/optional-alone" "$fixtures/optional-with-dep" "$fixtures/optional-lgx" || rc=1
exit "$rc"
