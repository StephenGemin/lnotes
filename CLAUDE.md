# lnotes

A minimal POSIX C CLI note manager. Notes are plain Markdown files with YAML frontmatter.
Keep it small, portable, and dependency-free.

## Build

```sh
make
make clean
make install   # installs to ~/.local/bin/notes
make test
make uninstall
```

## Constraints
* C99 or later; compile cleanly with both gcc and clang.
* Standard library and POSIX APIs only. No external libraries, no vendored code.
* No CMake, Autotools, Meson, pkg-config, Homebrew packages, or apt dependencies.
* Target: Linux and macOS. No GNU-only or platform-specific APIs.
* Avoid fixed-size path buffers; handle PATH_MAX carefully.
* No global mutable state.

## Storage
* Path precedence — keep this logic in one place, not scattered across commands:
  * LNOTES_DIR
  * XDG_DOCUMENTS_DIR/lnotes (Linux)
  * ~/Documents/lnotes
* Do not perform destructive operations unless the command explicitly requests it.
* Preserve existing Markdown body content when editing metadata.

## Note format

---
title: Example title
created: 2026-06-23T12:00:00Z
updated: 2026-06-23T12:00:00Z
---
Body follows the closing ---. Simple key/value parsing is sufficient; no full YAML parser.

# CLI conventions
Minimal, obvious commands:
```sh
notes add "Title" [-c <category>]
notes rm  <id|title>
notes ls  [-v] [-c <category>]
notes search <pattern> [-c <cat>] [-t] [-b]
```
Do not add flags or subcommands without a clear current need.
Do not rename commands, flags, or environment variables without being asked.
Error messages must be actionable: notes: could not open file: <path>, not just error.

## Code style
static for file-local functions.
Explicit, predictable ownership for allocated memory.
Clear names over abbreviations.

## Agent behavior
Before editing: git status, identify the smallest safe change.
Provide an initial plan before editing
Ask before: broad rewrites, changing the storage format, changing CLI behavior, or adding new commands or flags.
After editing: run make, then make test if tests exist. Report what ran and what didn't.

## Testing
Small POSIX shell tests or simple C tests via make test. Do not introduce a test framework.
New features require tests.

## Pull requests
Builds with make.
Tests pass.
New features have tests.
Diff is focused; no formatting churn, no build artifacts, no editor or cache files.
