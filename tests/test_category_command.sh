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

TEST_DIR=$(mktemp -d)
TEST_PREFIX="$TEST_DIR/prefix"
TEST_NOTES="$TEST_DIR/notes"
OBL="$TEST_PREFIX/bin/obl"

cleanup() { rm -rf "$TEST_DIR"; }
trap cleanup EXIT

cd "$PROJECT_ROOT"

PREFIX="$TEST_PREFIX" bash scripts/install.sh > /dev/null

run() { OBL_DIR="$TEST_NOTES" EDITOR=true "$OBL" "$@"; }

# ------------------------------------------------------------------ #
# cat add                                                             #
# ------------------------------------------------------------------ #
echo "cat add"

# First use seeds the five defaults (01..05); a new category appends as 06.
run cat add projects > /dev/null
assert_true "cat add appends as the next priority (06)" \
    sh -c "[ -d '$TEST_NOTES/06_projects' ]"

assert_true "cat add a duplicate name exits non-zero" \
    sh -c "! run cat add projects 2>/dev/null"

assert_true "cat add a default name exits non-zero" \
    sh -c "! run cat add general 2>/dev/null"

assert_true "cat add an invalid name exits non-zero" \
    sh -c "! run cat add foo/bar 2>/dev/null"

assert_true "cat add with no name exits non-zero" \
    sh -c "! run cat add 2>/dev/null"

# ------------------------------------------------------------------ #
# add into a missing category is an error (no auto-create)             #
# ------------------------------------------------------------------ #
echo "add guard"

assert_true "add into a missing category exits non-zero" \
    sh -c "! run add 'Whoops' -c ghost 2>/dev/null"

assert_true "add into a missing category creates no directory" \
    sh -c "! ls -d '$TEST_NOTES'/*_ghost 2>/dev/null"

# ------------------------------------------------------------------ #
# cat rm  (compacts the remaining priorities)                         #
# ------------------------------------------------------------------ #
echo "cat rm"

# Layout now: 01_general 02_software 03_media 04_personal_finance
#             05_medical 06_projects.  Remove the empty 02_software.
run cat rm software > /dev/null

assert_true "cat rm removes the category directory" \
    sh -c "[ ! -d '$TEST_NOTES/02_software' ]"

assert_true "cat rm shifts lower priorities up (media -> 02)" \
    sh -c "[ -d '$TEST_NOTES/02_media' ]"

assert_true "cat rm leaves no gap (no 06_ remains)" \
    sh -c "! ls -d '$TEST_NOTES'/06_* 2>/dev/null"

assert_true "cat rm renumbers the tail (projects -> 05)" \
    sh -c "[ -d '$TEST_NOTES/05_projects' ]"

# A removed category stays gone after a later add (no re-seeding).
run add "Fresh Note" > /dev/null
assert_true "removed default is not resurrected by a later add" \
    sh -c "! ls -d '$TEST_NOTES'/*_software 2>/dev/null"

# Refuse to remove a category that still holds notes.
run add "Held Note" -c general > /dev/null
assert_true "cat rm a non-empty category exits non-zero" \
    sh -c "! run cat rm general 2>/dev/null"

assert_true "cat rm a non-empty category keeps the directory" \
    sh -c "[ -d '$TEST_NOTES/01_general' ]"

assert_true "cat rm an unknown category exits non-zero" \
    sh -c "! run cat rm nosuchcat 2>/dev/null"

# ------------------------------------------------------------------ #
# dispatch                                                            #
# ------------------------------------------------------------------ #
echo "cat dispatch"

assert_true "cat with no action exits non-zero" \
    sh -c "! run cat 2>/dev/null"

assert_true "cat with an unknown action exits non-zero" \
    sh -c "! run cat frobnicate 2>/dev/null"

# ------------------------------------------------------------------ #
# Summary                                                              #
# ------------------------------------------------------------------ #
echo ""
echo "Results: $passes passed, $failures failed"
[ "$failures" -eq 0 ]
