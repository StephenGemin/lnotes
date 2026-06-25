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

cleanup() { rm -rf "$TEST_DIR"; }
trap cleanup EXIT

cd "$PROJECT_ROOT"

PREFIX="$TEST_PREFIX" bash scripts/install.sh > /dev/null

# ------------------------------------------------------------------ #
# view command                                                         #
# ------------------------------------------------------------------ #
echo "view"

add_out=$(OBL_DIR="$TEST_NOTES" EDITOR=true "$TEST_PREFIX/bin/obl" add "View Test Note" 2>&1)
view_id=$(echo "$add_out" | grep -oE '[0-9a-f]{8}' | head -1)

assert_nonempty "obl add returns an id" "$view_id"

assert_true "obl view by id exits 0" \
    sh -c "OBL_DIR='$TEST_NOTES' '$TEST_PREFIX/bin/obl' view '$view_id' >/dev/null"

assert_true "obl view by title exits 0" \
    sh -c "OBL_DIR='$TEST_NOTES' '$TEST_PREFIX/bin/obl' view 'View Test Note' >/dev/null"

OBL_DIR="$TEST_NOTES" "$TEST_PREFIX/bin/obl" view "$view_id" \
    > "$TEST_DIR/view_out.txt" 2>/dev/null

assert_true "obl view prints the title" \
    grep -q "View Test Note" "$TEST_DIR/view_out.txt"

assert_true "obl view prints the frontmatter id" \
    grep -q "id: $view_id" "$TEST_DIR/view_out.txt"

view_missing=$(OBL_DIR="$TEST_NOTES" "$TEST_PREFIX/bin/obl" view "no-such-note" 2>&1; echo "rc=$?")
assert_true "obl view unknown id exits non-zero" \
    sh -c "echo '$view_missing' | grep -q 'rc=1'"

assert_true "obl view missing arg exits non-zero" \
    sh -c "! OBL_DIR='$TEST_NOTES' '$TEST_PREFIX/bin/obl' view 2>/dev/null"

# ------------------------------------------------------------------ #
# Summary                                                              #
# ------------------------------------------------------------------ #
echo ""
echo "Results: $passes passed, $failures failed"
[ "$failures" -eq 0 ]
