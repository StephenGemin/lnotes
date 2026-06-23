# lnotes

A minimal command-line note manager. Notes are plain Markdown files with YAML
frontmatter, and can be opened in any editor or renderer.

Notes are stored under `~/Documents/lnotes`. On Linux, `XDG_DOCUMENTS_DIR` is
respected if set (`$XDG_DOCUMENTS_DIR/lnotes`).

## Install

**Requirements:** GCC or Clang, and a POSIX OS (Linux or macOS).

```sh
make install
```

This builds the app and copies the `notes` binary to `~/.local/bin/notes`.
Make sure `~/.local/bin` is on your `PATH`. If the `notes` command isn't found after installing, add this line to your shell config (`~/.bashrc`, `~/.zshrc`, etc.) and restart your terminal:

```sh
export PATH="$HOME/.local/bin:$PATH"
```

## Uninstall

```sh
make uninstall
```

You will be asked to confirm before anything is deleted. This removes the `notes` binary and your notes directory (`~/Documents/lnotes`). To skip the confirmation prompt:

```sh
make uninstall FORCE=1
```

## Commands

```
notes add <title> [-c <category>]            create a note and open in $EDITOR
notes rm  <id|title>                         remove a note (prompts for confirmation)
notes ls  [-v] [-c <category>]               list notes, newest first
notes search <pattern> [-c <cat>] [-t] [-b]  search notes (POSIX ERE, case-insensitive)
```

## Directory layout

```
~/Documents/lnotes/
  01_general/
  02_software/
  03_media/
  04_personal_finance/
  05_medical/
```

New categories are created automatically with the next available prefix.
Each note is a single file named `<id>_<slug>.md`.

## Configuration

| Variable            | Default               | Purpose                               |
|---------------------|-----------------------|---------------------------------------|
| `LNOTES_DIR`        | `~/Documents/lnotes`  | Override notes storage path           |
| `XDG_DOCUMENTS_DIR` | —                     | Respected on Linux if `LNOTES_DIR` unset |
| `EDITOR`            | `vi`                  | Editor opened by `notes add`          |
| `VISUAL`            | —                     | Checked before `EDITOR`               |
