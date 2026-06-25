#!/usr/bin/env bash
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

passes=0
failures=0

assert_true() {
    local desc="$1"; shift
    local rc=0
    "$@" 2>/dev/null || rc=$?
    if [ "$rc" -eq 0 ]; then
        passes=$((passes + 1))
        printf "  %-54s ok\n" "$desc"
    else
        failures=$((failures + 1))
        printf "  %-54s FAIL\n" "$desc"
    fi
}

assert_nonempty() {
    local desc="$1"
    local val="$2"
    if [ -n "$val" ]; then
        passes=$((passes + 1))
        printf "  %-54s ok\n" "$desc"
    else
        failures=$((failures + 1))
        printf "  %-54s FAIL\n" "$desc"
    fi
}

TEST_DIR=$(mktemp -d)
TEST_PREFIX="$TEST_DIR/prefix"
TEST_NOTES="$TEST_DIR/notes"
OBL="$TEST_PREFIX/bin/obl"

cleanup() { rm -rf "$TEST_DIR"; }
trap cleanup EXIT

cd "$PROJECT_ROOT"

PREFIX="$TEST_PREFIX" bash scripts/install.sh > /dev/null

# ------------------------------------------------------------------ #
# ls command                                                           #
# ------------------------------------------------------------------ #
echo "ls"

# Empty store
empty_out=$(OBL_DIR="$TEST_NOTES" "$OBL" ls 2>&1)
assert_true "obl ls on empty store reports no notes" \
    sh -c "echo '$empty_out' | grep -q 'No notes found'"

# Seed two notes in different categories (categories must exist first)
OBL_DIR="$TEST_NOTES" "$OBL" cat add work     >/dev/null 2>&1
OBL_DIR="$TEST_NOTES" "$OBL" cat add personal  >/dev/null 2>&1
OBL_DIR="$TEST_NOTES" EDITOR=true "$OBL" add "Alpha Note" -c work    >/dev/null 2>&1
OBL_DIR="$TEST_NOTES" EDITOR=true "$OBL" add "Beta Note"  -c personal >/dev/null 2>&1

list_out=$(OBL_DIR="$TEST_NOTES" "$OBL" ls 2>&1)

assert_true "obl ls shows the first note" \
    sh -c "echo '$list_out' | grep -q 'Alpha Note'"

assert_true "obl ls shows the second note" \
    sh -c "echo '$list_out' | grep -q 'Beta Note'"

assert_true "obl ls prints a header row" \
    sh -c "echo '$list_out' | grep -q 'Title'"

# Category filter
work_out=$(OBL_DIR="$TEST_NOTES" "$OBL" ls -c work 2>&1)
assert_true "obl ls -c work includes the work note" \
    sh -c "echo '$work_out' | grep -q 'Alpha Note'"
assert_true "obl ls -c work excludes the personal note" \
    sh -c "! echo '$work_out' | grep -q 'Beta Note'"

# Verbose flag still succeeds
assert_true "obl ls -v exits 0" \
    sh -c "OBL_DIR='$TEST_NOTES' '$OBL' ls -v >/dev/null"

# ------------------------------------------------------------------ #
# Summary                                                              #
# ------------------------------------------------------------------ #
echo ""
echo "Results: $passes passed, $failures failed"
[ "$failures" -eq 0 ]
