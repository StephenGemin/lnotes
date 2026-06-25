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

# Run obl against the test store. EDITOR=true keeps `add` non-interactive.
run() { OBL_DIR="$TEST_NOTES" EDITOR=true "$OBL" "$@"; }

# Find a note file by id, excluding the trash.
in_categories() { find "$TEST_NOTES" -path '*/.trash' -prune -o -name "$1" -print 2>/dev/null; }
in_trash()      { find "$TEST_NOTES/.trash" -name "$1" 2>/dev/null; }

# ------------------------------------------------------------------ #
# trash command                                                        #
# ------------------------------------------------------------------ #
echo "trash"

add_out=$(run add "Trash Test Note" 2>&1)
t_id=$(echo "$add_out" | grep -oE '[0-9a-f]{8}' | head -1)
assert_nonempty "obl add returns an id" "$t_id"

# Trashing moves the note out of its category and into .trash
run trash "$t_id" > /dev/null 2>&1
assert_true     "trashed note left its category" test -z "$(in_categories "${t_id}_*.md")"
assert_nonempty "trashed note is now in .trash"   "$(in_trash "${t_id}_*.md")"

# A trashed note is hidden from ls
ls_out=$(run ls 2>/dev/null)
assert_true "ls hides the trashed note" \
    sh -c "! echo '$ls_out' | grep -q '$t_id'"

# trash list surfaces it
list_out=$(run trash list 2>/dev/null)
assert_true "trash list shows the trashed note" \
    sh -c "echo '$list_out' | grep -q '$t_id'"

# trash restore returns it to its category
run trash restore "$t_id" > /dev/null 2>&1
assert_nonempty "restored note is back in a category" "$(in_categories "${t_id}_*.md")"
assert_true     "restored note is gone from .trash"   test -z "$(in_trash "${t_id}_*.md")"
ls_out2=$(run ls 2>/dev/null)
assert_true "ls shows the restored note" \
    sh -c "echo '$ls_out2' | grep -q '$t_id'"

# trash by title also works
run trash "Trash Test Note" > /dev/null 2>&1
assert_nonempty "trash by title moves the note to .trash" "$(in_trash "${t_id}_*.md")"

# trash clear empties the trash after confirmation
printf 'y\n' | run trash clear > /dev/null 2>&1
assert_true "trash clear empties the trash" test -z "$(in_trash '*.md')"

# trash clear on an empty trash reports cleanly and exits 0
empty_out=$(run trash clear 2>&1)
assert_true "trash clear on empty trash mentions empty" \
    sh -c "echo '$empty_out' | grep -qi 'empty'"
assert_true "trash clear on empty trash exits 0" \
    sh -c "OBL_DIR='$TEST_NOTES' '$OBL' trash clear >/dev/null"

# Error cases
assert_true "trash unknown id exits non-zero" \
    sh -c "! OBL_DIR='$TEST_NOTES' '$OBL' trash 'no-such-note' 2>/dev/null"

assert_true "trash restore unknown id exits non-zero" \
    sh -c "! OBL_DIR='$TEST_NOTES' '$OBL' trash restore 'no-such-note' 2>/dev/null"

assert_true "trash with no argument exits non-zero" \
    sh -c "! OBL_DIR='$TEST_NOTES' '$OBL' trash 2>/dev/null"

# ------------------------------------------------------------------ #
# Summary                                                              #
# ------------------------------------------------------------------ #
echo ""
echo "Results: $passes passed, $failures failed"
[ "$failures" -eq 0 ]
