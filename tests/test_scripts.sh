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

assert_true    "binary installed to PREFIX/bin/obl" \
               test -f "$TEST_PREFIX/bin/obl"

help_output=$("$TEST_PREFIX/bin/obl" -h 2>&1 || true)
assert_nonempty "obl -h produces output" "$help_output"

# ------------------------------------------------------------------ #
# Uninstall                                                            #
# ------------------------------------------------------------------ #
echo "uninstall"

mkdir -p "$TEST_NOTES/01_general"
touch "$TEST_NOTES/01_general/abc123_sample-note.md"

PREFIX="$TEST_PREFIX" OBL_DIR="$TEST_NOTES" bash scripts/uninstall.sh -f > /dev/null

assert_true "binary removed after uninstall" \
            test ! -f "$TEST_PREFIX/bin/obl"

assert_true "notes directory removed after uninstall" \
            test ! -d "$TEST_NOTES"

# ------------------------------------------------------------------ #
# Export                                                              #
# ------------------------------------------------------------------ #
echo "export"

EXPORT_NOTES="$TEST_DIR/export_notes"
EXPORT_OUT="$TEST_DIR/export_out"
mkdir -p "$EXPORT_NOTES/01_general" "$EXPORT_OUT"

cat > "$EXPORT_NOTES/01_general/abc12345_test-note.md" << 'EOF'
---
id: abc12345
title: Test Note
category: general
date: 2026-06-24
---

# Test Note
EOF

OBL_DIR="$EXPORT_NOTES" "$PROJECT_ROOT/build/obl" export "$EXPORT_OUT/backup" > /dev/null 2>&1

assert_true "export creates an archive file" \
            bash -c 'ls "$1"/backup.* 2>/dev/null | grep -q .' -- "$EXPORT_OUT"

ARCHIVE=$(ls "$EXPORT_OUT"/backup.* 2>/dev/null | head -1)
case "$ARCHIVE" in
    *.tar.gz) LIST_CMD="tar -tzf" ;;
    *.zip)    LIST_CMD="unzip -l" ;;
    *.tar)    LIST_CMD="tar -tf"  ;;
    *)        LIST_CMD="" ;;
esac

if [ -n "$LIST_CMD" ] && [ -n "$ARCHIVE" ]; then
    assert_true "archive contains note file" \
                bash -c '$1 "$2" 2>/dev/null | grep -q "\.md"' -- "$LIST_CMD" "$ARCHIVE"
fi

# ------------------------------------------------------------------ #
# Summary                                                              #
# ------------------------------------------------------------------ #
echo ""
echo "Results: $passes passed, $failures failed"
[ "$failures" -eq 0 ]
