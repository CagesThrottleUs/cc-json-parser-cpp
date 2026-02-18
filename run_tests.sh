#!/usr/bin/env bash
# Test runner: parallel batches, progress lines, final stats and failed list only.

set -uo pipefail
SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTS_DIR="${TESTS_DIR:-$SCRIPT_DIR/tests}"
PARSER="${PARSER:-$SCRIPT_DIR/out/build/clang-release/cc-json-parser-cpp}"
MAX_JOBS="${MAX_JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 8)}"

RESULTS_DIR="$(mktemp -d)"
trap 'rm -rf "$RESULTS_DIR"' EXIT

expected_exit() {
  local name="$1"
  case "$name" in
    invalid*) echo 1 ;;
    valid*)   echo 0 ;;
    fail*)    echo 1 ;;
    pass*)    echo 0 ;;
    i_*)      echo "0,1" ;;
    n_*)      echo 1 ;;
    y_*)      echo 0 ;;
    *)        return 1 ;;
  esac
}

run_one() {
  local f="$1" name expected got ok result
  name="$(basename "$f" .json)"
  expected="$(expected_exit "$name")" || {
    echo "Processing: $f" >&2
    echo "Done: $f -> SKIP" >&2
    echo "SKIP $f"
    return 0
  }
  echo "Processing: $f" >&2
  if "$PARSER" "$f" >/dev/null 2>&1; then got=0; else got=1; fi
  if [[ "$expected" == *","* ]]; then
    ok=0
    for e in ${expected//,/ }; do [[ "$e" == "$got" ]] && ok=1; done
  else
    [[ "$expected" == "$got" ]] && ok=1 || ok=0
  fi
  if [[ "$ok" -eq 1 ]]; then result="PASS"; else result="FAIL"; fi
  echo "Done: $f -> $result" >&2
  echo "$result $f"
}

if [[ ! -x "$PARSER" ]]; then
  echo "Parser not found: $PARSER" >&2
  exit 1
fi

total_files="$(find "$TESTS_DIR" -type f -name '*.json' | wc -l | tr -d ' ')"
echo "Running $total_files tests (parallel: $MAX_JOBS)..." >&2

idx=0
while IFS= read -r -d '' f; do
  idx=$((idx + 1))
  ( run_one "$f" >"$RESULTS_DIR/$idx" ) &
  if [[ $((idx % MAX_JOBS)) -eq 0 ]]; then
    wait 2>/dev/null || true
  fi
done < <(find "$TESTS_DIR" -type f -name '*.json' -print0)
wait 2>/dev/null || true

# Aggregate with find (avoids "argument list too long" with 368 files)
passed=0 failed=0 skipped=0
while IFS= read -r line; do
  case "$line" in
    PASS*) passed=$((passed + 1)) ;;
    FAIL*) failed=$((failed + 1)) ;;
    SKIP*) skipped=$((skipped + 1)) ;;
  esac
done < <(find "$RESULTS_DIR" -type f -exec cat {} \; 2>/dev/null)

echo ""
echo "--- Failed tests ---"
failed_list="$(find "$RESULTS_DIR" -type f -exec cat {} \; 2>/dev/null | sed -n 's/^FAIL /FAIL: /p')"
if [[ -n "$failed_list" ]]; then
  echo "$failed_list"
else
  echo "(none)"
fi
echo ""
echo "Passed:  $passed"
echo "Failed:  $failed"
echo "Skipped: $skipped"
echo "Total:   $((passed + failed + skipped))"
echo ""
if [[ "$failed" -eq 0 ]]; then
  echo "Result: PASS"
  exit 0
else
  echo "Result: FAILED"
  exit 1
fi
