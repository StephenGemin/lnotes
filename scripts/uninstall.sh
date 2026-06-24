#!/usr/bin/env bash
set -euo pipefail

FORCE=0
while getopts "f" opt; do
    case $opt in
        f) FORCE=1 ;;
        *) echo "Usage: $0 [-f]" >&2; exit 1 ;;
    esac
done

PREFIX="${PREFIX:-$HOME/.local}"
BINARY="$PREFIX/bin/obl"

# Mirrors get_notes_dir() in src/utils.c
if [ -n "${OBL_DIR:-}" ]; then
    NOTES_DIR="$OBL_DIR"
elif [ -n "${XDG_DOCUMENTS_DIR:-}" ]; then
    NOTES_DIR="$XDG_DOCUMENTS_DIR/oubliette"
else
    NOTES_DIR="$HOME/Documents/oubliette"
fi

BINARY_EXISTS=0
NOTES_EXISTS=0
[ -f "$BINARY" ] && BINARY_EXISTS=1
[ -d "$NOTES_DIR" ] && NOTES_EXISTS=1

if [ $BINARY_EXISTS -eq 0 ] && [ $NOTES_EXISTS -eq 0 ]; then
    echo "Nothing to uninstall: binary and notes directory not found."
    exit 0
fi

if [ $FORCE -eq 0 ]; then
    echo "The following will be permanently deleted:"
    [ $BINARY_EXISTS -eq 1 ] && echo "  Binary:  $BINARY"
    [ $NOTES_EXISTS -eq 1 ] && echo "  Notes:   $NOTES_DIR"
    printf "Are you sure? [y/N] "
    read -r answer
    case "$answer" in
        [yY]) ;;
        *) echo "Aborted."; exit 0 ;;
    esac
fi

if [ $BINARY_EXISTS -eq 1 ]; then
    rm -f "$BINARY"
    echo "Removed $BINARY"
else
    echo "Warning: binary not found at $BINARY, skipping."
fi

if [ $NOTES_EXISTS -eq 1 ]; then
    rm -rf "$NOTES_DIR"
    echo "Removed $NOTES_DIR"
else
    echo "Warning: notes directory not found at $NOTES_DIR, skipping."
fi

echo "Uninstall complete."
