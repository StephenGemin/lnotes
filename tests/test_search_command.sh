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
# search command  (search exits 0 even on no match; assert on output)  #
# ------------------------------------------------------------------ #
echo "search"

# Categories must exist before notes can be filed under them
OBL_DIR="$TEST_NOTES" "$OBL" cat add work     >/dev/null 2>&1
OBL_DIR="$TEST_NOTES" "$OBL" cat add personal  >/dev/null 2>&1

add_out=$(OBL_DIR="$TEST_NOTES" EDITOR=true "$OBL" add "Searchable Alpha" -c work 2>&1)
note_id=$(echo "$add_out" | grep -oE '[0-9a-f]{8}' | head -1)
note_file=$(find "$TEST_NOTES" -name "${note_id}_*.md" | head -1)

# Inject a unique token into the body only (title has no such token)
printf 'a unique zzbodytoken line\n' >> "$note_file"

# Default search hits the title
title_out=$(OBL_DIR="$TEST_NOTES" "$OBL" search "Searchable" 2>&1)
assert_true "search matches a title term" \
    sh -c "echo '$title_out' | grep -q 'Searchable Alpha'"

# Body-only flag finds the body token
body_out=$(OBL_DIR="$TEST_NOTES" "$OBL" search "zzbodytoken" -b 2>&1)
assert_true "search -b matches a body term" \
    sh -c "echo '$body_out' | grep -q 'Searchable Alpha'"

# Title-only flag does not match a body-only token
title_only_out=$(OBL_DIR="$TEST_NOTES" "$OBL" search "zzbodytoken" -t 2>&1)
assert_true "search -t ignores body-only matches" \
    sh -c "echo '$title_only_out' | grep -q 'No matches found'"

# No match reports cleanly
none_out=$(OBL_DIR="$TEST_NOTES" "$OBL" search "qqzznomatch" 2>&1)
assert_true "search with no match reports cleanly" \
    sh -c "echo '$none_out' | grep -q 'No matches found'"

# Category filter scopes results
OBL_DIR="$TEST_NOTES" EDITOR=true "$OBL" add "Searchable Beta" -c personal >/dev/null 2>&1
cat_out=$(OBL_DIR="$TEST_NOTES" "$OBL" search "Searchable" -c work 2>&1)
assert_true "search -c includes the in-category note" \
    sh -c "echo '$cat_out' | grep -q 'Searchable Alpha'"
assert_true "search -c excludes out-of-category notes" \
    sh -c "! echo '$cat_out' | grep -q 'Searchable Beta'"

# Missing pattern
assert_true "search with no pattern exits non-zero" \
    sh -c "! OBL_DIR='$TEST_NOTES' '$OBL' search 2>/dev/null"

# ------------------------------------------------------------------ #
# Summary                                                              #
# ------------------------------------------------------------------ #
echo ""
echo "Results: $passes passed, $failures failed"
[ "$failures" -eq 0 ]
