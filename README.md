# lnotes

A minimal command-line note manager. Notes are plain Markdown files with YAML
frontmatter, stored locally under `~/Documents/lnotes`. No cloud, no database.

## Requirements

- C99 compiler (GCC or Clang)
- POSIX OS (Linux or macOS)

## Build & Install

```sh
make          # build
make test     # run unit tests
make install  # install to ~/.local/bin/notes
```

Override the install prefix:

```sh
make install PREFIX=/usr/local
```

## Usage

```
notes add <title> [-c <category>]
notes rm  <id|title>
notes ls  [-v] [-c <category>]
notes search <pattern> [-c <category>] [-t] [-b]
```

### Add a note

```sh
notes add "Rust Async Patterns" -c software
notes add "Doctor visit notes" -c medical
notes add "Movie watchlist"                   # defaults to general
```

Opens `$EDITOR` (falls back to `vi`) after creating the file.

### List notes

```sh
notes ls                    # all notes, newest first
notes ls -c software        # filter by category
notes ls -v                 # include first body line
notes ls -v -c media        # combine flags
```

```
ID        Title                             Category              Date
--------  --------------------------------  --------------------  ----------
a3f9c12b  Rust Async Patterns               software              2026-06-23
b7e4d901  Doctor visit notes                medical               2026-06-20
```

### Search notes

Patterns are POSIX extended regular expressions, matched case-insensitively.

```sh
notes search "tokio|async"             # title and body (default)
notes search "tokio" -c software       # restrict to a category
notes search "tokio" -t                # title only
notes search "spawn.*task" -b          # body only
```

Body matches print the first matching line:

```
a3f9c12b  Rust Async Patterns               software              2026-06-23
          > Using tokio::spawn to run concurrent tasks concurrently.
```

### Remove a note

```sh
notes rm a3f9c12b                      # by ID
notes rm "Rust Async Patterns"         # by title (case-insensitive)
```

Prompts for confirmation before deleting.

## Directory layout

```
~/Documents/lnotes/
  01_general/
  02_software/
  03_media/
  04_personal_finance/
  05_medical/
```

New categories are created automatically with the next available prefix:

```sh
notes add "Sourdough starter" -c recipes   # creates 06_recipes/
```

Each note is a single file named `<id>_<slug>.md`:

```
02_software/a3f9c12b_rust-async-patterns.md
```

## Note format

```markdown
---
id: a3f9c12b
title: Rust Async Patterns
category: software
date: 2026-06-23
---

# Rust Async Patterns

Notes go here in plain Markdown.
```

The file is ordinary Markdown and can be opened in any editor or renderer.

## Configuration

| Variable       | Default                    | Purpose                        |
|----------------|----------------------------|--------------------------------|
| `LNOTES_DIR`   | `~/Documents/lnotes`       | Override notes storage path    |
| `EDITOR`       | `vi`                       | Editor opened by `notes add`   |
| `VISUAL`       | —                          | Checked before `EDITOR`        |
