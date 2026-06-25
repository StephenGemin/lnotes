# oubliette

A minimal POSIX C CLI note manager. Notes are plain Markdown files with YAML frontmatter.
Keep it small, portable, and dependency-free.

## Project structure

```
src/main.c      CLI entry point and command dispatch
src/notes.c     collect_all_notes, cmd_add, cmd_remove, cmd_list, cmd_category
src/notes.h     shared Note struct, constants, all declarations
src/search.c    cmd_search
src/utils.c     get_notes_dir (storage path logic), parse_frontmatter,
                generate_id, title_to_slug, open_in_editor
tests/                 C unit tests and shell integration tests (one file per module/command)
scripts/install.sh     install logic
scripts/uninstall.sh   uninstall logic
```

## Philosophy

oubliette is intentionally small. Every change should leave it simpler or more correct,
not more capable. If a feature request can be satisfied without new code, prefer that.

## Making changes

Start with the smallest change that solves the problem. Do not refactor, rename, or
reformat code adjacent to the fix unless it is directly in the way.

Before touching storage logic, CLI behavior, or the note format, confirm the change
is explicitly wanted. These are the highest-impact surfaces:

- **Storage path logic** lives in one place (`get_notes_dir` in `src/utils.c`). Do not scatter it across commands.
- **CLI commands, flags, and environment variables** must not be silently renamed or removed.
- **Note frontmatter keys** must remain stable. Existing notes in the wild must still parse.
- **Markdown body content** must be preserved when editing metadata.

## What requires a conversation first

- Changing the storage format or path resolution logic.
- Renaming, removing, or changing the behavior of any command or flag.
- Adding a new command or flag.
- Adding any dependency outside the C standard library and POSIX.
- Broad rewrites or restructuring across multiple files.

## Git history

Keep a clean, linear history. When a branch conflicts with its base, prefer rebase
over merge. Attempt to resolve the conflicts. If resolution is ambiguous or risks
discarding intent on either side, stop, explain the conflict, and ask for direction
rather than guessing.

## Build and test

```sh
make          # must compile cleanly with both gcc and clang
make clean
make install  # installs to ~/.local/bin/obl
make test
make uninstall
```

Tests should be small POSIX shell scripts or simple C programs. Do not introduce a
test framework. New features require tests.

## Storage

Path precedence — keep this logic in one place, not scattered across commands:
1. `OBL_DIR`
2. `XDG_DOCUMENTS_DIR/oubliette` (Linux)
3. `~/Documents/oubliette`

Do not perform destructive operations unless the command explicitly requests it.

## Note format

```yaml
---
id: a1b2c3d4
title: Example title
category: general
date: 2026-06-23
---
```

Keys, as written by `obl add`:

- `id` — 8-character hex identifier used to address the note.
- `title` — the note's title.
- `category` — category name, without the directory's numeric prefix.
- `date` — creation date, `YYYY-MM-DD` (local time).

Body follows the closing `---`. Simple key/value parsing is sufficient; no full YAML
parser. Unknown keys are ignored, so notes may carry extra frontmatter without breaking.

## CLI

Minimal, obvious commands:
```sh
obl add   "Title" [-c <category>]
obl raise <id|title>
obl rm    <id|title>
obl ls    [-v] [-c <category>]
obl search <pattern> [-c <cat>] [-t] [-b]
obl cat   <add|rm> <name>
```

Do not add flags or subcommands without a clear current need.
Error messages must be actionable: `obl: could not open file: <path>`, not just `error`.

## Code style

- `static` for file-local functions.
- Explicit, predictable ownership for allocated memory.
- Clear names over abbreviations.

## What to avoid

- GNU-only APIs, Linux-only paths, or macOS-only APIs.
- Fixed-size path buffers without care for `PATH_MAX`.
- Global mutable state.
- External libraries, vendored code, or build systems beyond the existing Makefile.
- Committing build artifacts, editor files, or cache files.
