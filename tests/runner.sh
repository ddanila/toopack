#!/bin/sh
# tests/runner.sh — build and run every test in tests/*/, print a summary.
#
# Each test's makefile must expose a `test` target that prints exactly one
# result line: "<name>             PASS|FAIL  ( <bytes> B)". The runner
# captures everything but only echoes the result line into the summary, so
# build chatter (pack.sh ratios, wcc lines) doesn't pollute the table.

set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT/tests"

passed=0
failed=0
results=""
log=$(mktemp)

for d in */; do
    name=${d%/}
    [ -f "$d/makefile" ] || continue
    grep -q "^test:" "$d/makefile" || continue

    if (cd "$d" && make -s test) > "$log" 2>&1; then
        passed=$((passed + 1))
    else
        failed=$((failed + 1))
    fi
    line=$(grep -E "PASS|FAIL" "$log" | tail -1)
    if [ -z "$line" ]; then
        line="$name             ?  (build error)"
        cat "$log"
    fi
    results="${results}${line}
"
done
rm -f "$log"

printf '\n==== toopack tests ====\n'
printf '%s' "$results"
printf '\npassed: %d   failed: %d\n' "$passed" "$failed"

[ "$failed" -eq 0 ]
