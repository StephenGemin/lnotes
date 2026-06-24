# Development guide

## Philosophy

oubliette is intentionally small. Every change should leave it simpler or more correct,
not more capable. If a feature request can be satisfied without new code, prefer that.

## Making changes

Start with the smallest change that solves the problem. Do not refactor, rename, or
reformat code adjacent to the fix unless it is directly in the way.

Before touching storage logic, CLI behavior, or the note format, confirm the change
is explicitly wanted. These are the highest-impact surfaces:

- **Storage path logic** lives in one place. Do not scatter it across commands.
- **CLI commands, flags, and environment variables** must not be silently renamed or removed.
- **Note frontmatter keys** must remain stable. Existing notes in the wild must still parse.
- **Markdown body content** must be preserved when editing metadata.

## Build and test

```sh
make          # must compile cleanly with both gcc and clang
make test     # run if tests exist; new features require new tests
```
Tests should be small POSIX shell scripts or simple C programs. Do not introduce a
test framework.

## What requires a conversation first
- Changing the storage format or path resolution logic.
- Renaming, removing, or changing the behavior of any command or flag.
- Adding a new command or flag.
- Adding any dependency outside the C standard library and POSIX.
- Broad rewrites or restructuring across multiple files.

## What to avoid
- GNU-only APIs, Linux-only paths, or macOS-only APIs.
- Fixed-size path buffers without care for `PATH_MAX`.
- Global mutable state.
- External libraries, vendored code, or build systems beyond the existing Makefile.
- Committing build artifacts, editor files, or cache files.
