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

cleanup() { rm -rf "$TEST_DIR"; }
trap cleanup EXIT

cd "$PROJECT_ROOT"

PREFIX="$TEST_PREFIX" bash scripts/install.sh > /dev/null

# Seed the notes directory with a couple of note files
mkdir -p "$TEST_NOTES/01_general" "$TEST_NOTES/02_software"
cat > "$TEST_NOTES/01_general/abc12345_first-note.md" << 'EOF'
---
id: abc12345
title: First Note
category: general
date: 2026-06-24
---

# First Note
EOF
cat > "$TEST_NOTES/02_software/def67890_second-note.md" << 'EOF'
---
id: def67890
title: Second Note
category: software
date: 2026-06-24
---

# Second Note
EOF

# ------------------------------------------------------------------ #
# export command                                                       #
# ------------------------------------------------------------------ #
echo "export"

# --- custom output base ---
EXPORT_OUT="$TEST_DIR/export_out"
mkdir -p "$EXPORT_OUT"

OBL_DIR="$TEST_NOTES" "$TEST_PREFIX/bin/obl" export "$EXPORT_OUT/backup" > /dev/null 2>&1

assert_true "export with output base creates an archive" \
    bash -c 'ls "$1"/backup.* 2>/dev/null | grep -q .' -- "$EXPORT_OUT"

ARCHIVE=$(ls "$EXPORT_OUT"/backup.* 2>/dev/null | head -1)

assert_true "archive contains note files" \
    bash -c '
        case "$1" in
            *.tar.gz) tar -tzf "$1" ;;
            *.zip)    unzip -l  "$1" ;;
            *.tar)    tar -tf   "$1" ;;
        esac 2>/dev/null | grep -q "\.md"
    ' -- "$ARCHIVE"

# --- default output base (dated filename in cwd) ---
DEFAULT_OUT="$TEST_DIR/default_out"
mkdir -p "$DEFAULT_OUT"

default_export=$(cd "$DEFAULT_OUT" && OBL_DIR="$TEST_NOTES" "$TEST_PREFIX/bin/obl" export 2>/dev/null)

assert_true "export with no args prints output path" \
    bash -c '[ -n "$1" ]' -- "$default_export"

assert_true "export with no args creates archive in cwd" \
    bash -c 'ls "$1"/oubliette-*.* 2>/dev/null | grep -q .' -- "$DEFAULT_OUT"

# --- output path is printed on success ---
assert_true "export prints path of created archive" \
    bash -c '
        out=$(OBL_DIR="$1" "$2" export "$3/printed" 2>/dev/null)
        echo "$out" | grep -q "exported:"
    ' -- "$TEST_NOTES" "$TEST_PREFIX/bin/obl" "$EXPORT_OUT"

# --- non-existent notes dir returns non-zero ---
assert_true "export with missing notes dir exits non-zero" \
    bash -c '! OBL_DIR="$1/no-such-dir" "$2" export "$3/should-not-exist" 2>/dev/null' \
    -- "$TEST_DIR" "$TEST_PREFIX/bin/obl" "$EXPORT_OUT"

# ------------------------------------------------------------------ #
# Summary                                                              #
# ------------------------------------------------------------------ #
echo ""
echo "Results: $passes passed, $failures failed"
[ "$failures" -eq 0 ]
