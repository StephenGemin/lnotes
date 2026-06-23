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

# ------------------------------------------------------------------ #
# Install                                                              #
# ------------------------------------------------------------------ #
echo "install"

PREFIX="$TEST_PREFIX" bash scripts/install.sh > /dev/null

assert_true    "binary installed to PREFIX/bin/notes" \
               test -f "$TEST_PREFIX/bin/notes"

help_output=$("$TEST_PREFIX/bin/notes" -h 2>&1 || true)
assert_nonempty "notes -h produces output" "$help_output"

# ------------------------------------------------------------------ #
# Uninstall                                                            #
# ------------------------------------------------------------------ #
echo "uninstall"

mkdir -p "$TEST_NOTES/01_general"
touch "$TEST_NOTES/01_general/abc123_sample-note.md"

PREFIX="$TEST_PREFIX" LNOTES_DIR="$TEST_NOTES" bash scripts/uninstall.sh -f > /dev/null

assert_true "binary removed after uninstall" \
            test ! -f "$TEST_PREFIX/bin/notes"

assert_true "notes directory removed after uninstall" \
            test ! -d "$TEST_NOTES"

# ------------------------------------------------------------------ #
# Summary                                                              #
# ------------------------------------------------------------------ #
echo ""
echo "Results: $passes passed, $failures failed"
[ "$failures" -eq 0 ]
