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
# add command                                                          #
# ------------------------------------------------------------------ #
echo "add"

add_out=$(OBL_DIR="$TEST_NOTES" EDITOR=true "$OBL" add "Add Test Note" 2>&1)
add_id=$(echo "$add_out" | grep -oE '[0-9a-f]{8}' | head -1)

assert_nonempty "obl add returns an 8-char id" "$add_id"

assert_true "obl add reports the created note" \
    sh -c "echo '$add_out' | grep -q 'Created note'"

note_file=$(find "$TEST_NOTES" -name "${add_id}_*.md" | head -1)
assert_nonempty "obl add writes a note file to disk" "$note_file"

assert_true "note file has title frontmatter" \
    sh -c "grep -q '^title: Add Test Note' '$note_file'"

assert_true "default category note lands in general dir" \
    sh -c "echo '$note_file' | grep -q '/[0-9][0-9]_general/'"

# Unknown category is an error; nothing is created
assert_true "obl add -c unknown category exits non-zero" \
    sh -c "! OBL_DIR='$TEST_NOTES' EDITOR=true '$OBL' add 'Nope' -c nosuchcat 2>/dev/null"

assert_true "obl add -c unknown category creates no directory" \
    sh -c "! ls -d '$TEST_NOTES'/*_nosuchcat 2>/dev/null"

# Custom category works once it has been created explicitly
OBL_DIR="$TEST_NOTES" "$OBL" cat add projects > /dev/null
cat_out=$(OBL_DIR="$TEST_NOTES" EDITOR=true "$OBL" add "Custom Cat Note" -c projects 2>&1)
cat_id=$(echo "$cat_out" | grep -oE '[0-9a-f]{8}' | head -1)
cat_file=$(find "$TEST_NOTES" -name "${cat_id}_*.md" | head -1)

assert_true "obl add -c places note in an existing projects dir" \
    sh -c "echo '$cat_file' | grep -q '_projects/'"

# Missing title
assert_true "obl add with no title exits non-zero" \
    sh -c "! OBL_DIR='$TEST_NOTES' EDITOR=true '$OBL' add 2>/dev/null"

# ------------------------------------------------------------------ #
# Summary                                                              #
# ------------------------------------------------------------------ #
echo ""
echo "Results: $passes passed, $failures failed"
[ "$failures" -eq 0 ]
