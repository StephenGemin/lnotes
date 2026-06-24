# oubliette

A minimal command-line note manager. A place to offload things you don't want
to be responsible for remembering, kept local and offline. Notes are plain
Markdown files with YAML frontmatter, and can be opened in any editor or renderer.

Notes are stored under `~/Documents/oubliette`. On Linux, `XDG_DOCUMENTS_DIR` is
respected if set (`$XDG_DOCUMENTS_DIR/oubliette`).

## Install

**Requirements:** GCC or Clang, and a POSIX OS (Linux or macOS).

```sh
make install
```

This builds the app and copies the `obl` binary to `~/.local/bin/obl`.
Make sure `~/.local/bin` is on your `PATH`. If the `obl` command isn't found after installing, add this line to your shell config (`~/.bashrc`, `~/.zshrc`, etc.) and restart your terminal:

```sh
export PATH="$HOME/.local/bin:$PATH"
```

## Uninstall

```sh
make uninstall
```

You will be asked to confirm before anything is deleted. This removes the `obl` binary and your notes directory (`~/Documents/oubliette`). To skip the confirmation prompt:

```sh
make uninstall FORCE=1
```

## Commands

```
obl add    <title> [-c <category>]            create a note and open in $EDITOR
obl rm     <id|title>                         remove a note (prompts for confirmation)
obl ls     [-v] [-c <category>]               list notes, newest first
obl search <pattern> [-c <cat>] [-t] [-b]     search notes (POSIX ERE, case-insensitive)
obl export [output-base]                       archive all notes to a file
```

`obl export` tries `tar.gz`, then `zip`, then plain `tar`, and uses whichever archive
tool is available. The output file gets the appropriate extension appended
(e.g. `obl export /mnt/usb/backup` → `backup.tar.gz`). Omitting the argument
creates `oubliette-YYYY-MM-DD.tar.gz` (or equivalent) in the current directory.

## Directory layout

```
~/Documents/oubliette/
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
| `OBL_DIR`           | `~/Documents/oubliette` | Override notes storage path         |
| `XDG_DOCUMENTS_DIR` | —                       | Respected on Linux if `OBL_DIR` unset |
| `EDITOR`            | `vi`                    | Editor opened by `obl add`           |
| `VISUAL`            | —                     | Checked before `EDITOR`               |
