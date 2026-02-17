#!/usr/bin/env bash
# Full test-suite validation: run parser on all tests/*.json and check exit code by name.

set -euo pipefail
SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTS_DIR="${TESTS_DIR:-$SCRIPT_DIR/tests}"
PARSER="${PARSER:-$SCRIPT_DIR/out/build/clang-release/cc-json-parser-cpp}"
RESULTS_DIR="$(mktemp -d)"
trap 'rm -rf "$RESULTS_DIR"' EXIT

expected_exit() {
  local name="$1"
  case "$name" in
    invalid*) echo 1 ;;
    valid*)   echo 0 ;;
    fail*)    echo 1 ;;
    pass*)    echo 0 ;;
    i_*)      echo "0,1" ;;   # invalid: accept 0 or 1
    n_*)      echo 1 ;;
    y_*)      echo 0 ;;
    *)        return 1 ;;
  esac
}

run_one() {
  local f="$1" name expected got ok
  name="$(basename "$f" .json)"
  expected="$(expected_exit "$name")" || return 0  # skip unknown prefix
  if "$PARSER" "$f" >/dev/null 2>&1; then got=0; else got=1; fi
  if [[ "$expected" == *","* ]]; then
    ok=0
    for e in ${expected//,/ }; do [[ "$e" == "$got" ]] && ok=1; done
  else
    [[ "$expected" == "$got" ]] && ok=1 || ok=0
  fi
  if [[ "$ok" -eq 1 ]]; then echo "PASS $f"; else echo "FAIL $f"; fi
}

export -f expected_exit run_one
export PARSER

if [[ ! -x "$PARSER" ]]; then
  echo "Parser not found or not executable: $PARSER" >&2
  exit 1
fi

MAX_JOBS="${MAX_JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 8)}"
export RESULTS_DIR
find "$TESTS_DIR" -type f -name '*.json' -print0 | xargs -0 -P "$MAX_JOBS" -I {} bash -c 'run_one "{}" > "$RESULTS_DIR/$$.$RANDOM"'

passed=0
failed=0
for r in "$RESULTS_DIR"/*; do
  [[ -f "$r" ]] || continue
  case "$(cat "$r")" in
    PASS*) passed=$((passed + 1)) ;;
    FAIL*) failed=$((failed + 1)) ;;
  esac
done
count=$((passed + failed))

echo "--- Results ---"
for r in "$RESULTS_DIR"/*; do
  [[ -f "$r" ]] && grep -q '^FAIL' "$r" && sed 's/^FAIL /FAIL: /' "$r"
done
echo "Passed: $passed"
echo "Failed: $failed"
echo "Total:  $count"

[[ "$failed" -eq 0 ]]
