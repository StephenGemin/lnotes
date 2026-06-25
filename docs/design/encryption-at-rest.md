# Design: opt-in encryption-at-rest

- **Status:** proposed (design agreed; no implementation yet)
- **Tracking issue:** [#15](https://github.com/StephenGemin/oubliette/issues/15)
- **Scope of this document:** the design we will build against. It records the
  decisions, the reasoning, and the honest limitations. It is not a commitment
  to a particular code layout.

## Summary

oubliette will gain **opt-in encryption-at-rest**. Plaintext Markdown stays the
default and existing notes are never touched. A user may additionally mark a
note as encrypted; that note is stored as an opaque, authenticated-encrypted
blob whose filename reveals nothing. Encryption is a build-time opt-in
(`make ENABLE_CRYPTO=1`) so the default build remains dependency-free.

Three decisions anchor everything below:

1. **Hide everything.** An encrypted note hides its entire content — title,
   category, date, and body — not just the body.
2. **Passphrase you type.** The key is derived from a passphrase held in your
   head. No key file is stored on disk.
3. **libsodium, behind an optional build flag.** The cryptography is delegated
   to the audited [libsodium](https://libsodium.org) library, linked only when
   built with `ENABLE_CRYPTO=1`.

## Terminology (plain language)

This project is also a vehicle for learning the surrounding security concepts,
so the terms are defined here rather than assumed.

- **Plaintext / ciphertext** — the readable note vs. the scrambled blob on disk.
- **Symmetric encryption** — one secret (derived from the passphrase) both locks
  and unlocks. This is what we use.
- **AEAD** (*Authenticated Encryption with Associated Data*) — encryption that
  also detects tampering. A flipped byte or a corrupt backup makes decryption
  fail loudly instead of returning garbage. The modern default.
- **KDF** (*Key Derivation Function*, here **Argon2**) — turns a human passphrase
  into a proper key, deliberately slowly, so guessing passphrases by brute force
  is expensive. libsodium's `crypto_pwhash` is Argon2.
- **Nonce** (*number used once*) — mixed into each encryption so identical
  plaintext does not produce identical ciphertext. Reuse is a serious bug;
  libsodium's high-level APIs manage it for us.
- **Salt** — random, non-secret bytes stored with the ciphertext so the same
  passphrase does not derive the same key across notes or machines.

The primitives are **not** implemented here. libsodium implements them; our job
is to call them correctly, which is where real-world security bugs actually live.

## Threat model

### What encryption-at-rest defends against

An attacker who **has the files but not your running, unlocked session**:

- a lost or stolen device without full-disk encryption;
- a backup drive or a Time Machine snapshot;
- a cloud-sync copy (Dropbox / iCloud / Drive);
- an `obl export` archive dropped into a shared or synced location;
- blocks recoverable on disk after `obl rm` (which only unlinks).

Against all of these, "hide everything" means the attacker sees random-named
blobs and learns nothing — not even note titles.

### What it deliberately does NOT defend against

Stating these plainly is part of the design:

- **Malware or another user running as you while a note is decrypted.** An open
  note is plaintext in memory and, briefly, in a RAM-backed temp file. No
  at-rest scheme can help here.
- **A weak passphrase.** The KDF slows guessing; it does not stop it. The
  passphrase strength *is* the security ceiling.
- **Editor leftovers.** Swap files, undo history, and `.viminfo` are the most
  commonly overlooked plaintext leak. The editor flow below exists to contain
  them; it is the hardest part of this design, and it is not the cipher.
- **Loss of the passphrase.** There is no recovery and no backdoor. Forgetting
  the passphrase means the notes are permanently unreadable. That is the
  guarantee, not a bug.

## Decisions and rationale

### 1. Granularity: per-note

A note is **either** an ordinary plaintext `.md` (unchanged) **or** an encrypted
blob. This is the only granularity consistent with "opt-in, default plaintext,
existing notes untouched":

- whole-vault would make encryption the default — it is not;
- per-category is awkward because categories here are fuzzy, substring-matched
  directories with numeric prefixes.

A future convenience (e.g. "new notes in `05_medical` default to encrypted") can
be layered on later; it is not foundational.

### 2. Mechanism: libsodium behind `make ENABLE_CRYPTO=1`

- **Not hand-rolled.** For data this sensitive, hand-written crypto is dangerous
  (timing side-channels, nonce reuse, weak KDFs). The primitives can be learned
  in a throwaway sandbox; they will not ship here. libsodium is the standard
  "don't roll your own" answer in C.
- **Not gpg / `openssl enc`.** OpenPGP and `openssl enc` carry format baggage and
  weak-by-default settings — poor UX and poor learning material.
- **Why libsodium over shelling out to `age`.** `age` is genuinely good and would
  have matched how `export` already shells out to `tar`/`zip`. The deciding
  factor is that oubliette is a **stateless CLI**: it exits after every command,
  so there is no agent to hold an unlocked key between runs. With `age`
  passphrase mode, a `search` across N encrypted notes prompts N times (age
  requires a TTY per invocation). In-process libsodium prompts **once per
  command** and decrypts all N in memory, can wipe the key from RAM
  (`sodium_memzero`), and never hands plaintext to another process.
- **The optional flag contains the cost.** The default `make` stays 100%
  dependency-free and builds an `obl` with encryption disabled; the encryption
  commands then print a clear "rebuild with `ENABLE_CRYPTO=1`" message. Only the
  opt-in build links libsodium, which must compile cleanly under gcc and clang.

### 3. Key management: passphrase, no daemon

The user types a passphrase; Argon2 stretches it into the key; the key lives in
process memory for one command only, then is zeroed. No key file to leak,
nothing extra in a backup. No background agent — a daemon holding the key would
be a new, always-on attack surface and would fight the project's minimalism. The
cost, accepted deliberately: re-typing per command, and no recovery if forgotten.

## On-disk format

- Plaintext `.md` notes: **unchanged.** Frontmatter keys stay stable; existing
  notes parse exactly as today. The stability contract is about plaintext notes,
  and they do not change.
- Encrypted note: an **opaque blob** named `<id>.obl` — the random 8-hex id only,
  **no title slug** (the slug would leak the title we chose to hide). The blob
  header carries the salt, KDF parameters, and nonce required to decrypt; the
  payload is AEAD ciphertext.
- Decrypting a blob yields a **byte-identical ordinary oubliette note**
  (`---`-delimited frontmatter + body). We do not invent a new note format; we
  only change the at-rest representation. The frontmatter contract is preserved
  for the recovered content.

`collect_all_notes` (today globs `*.md`) learns to also see `<id>.obl` files so
that `ls` and `rm` know they exist, **without** feeding ciphertext to
`parse_frontmatter`. Encrypted entries take a separate code path.

## Command behavior

### `ls`
Lists encrypted notes without a passphrase, shown as `🔒 [encrypted]` with their
id so you know they exist. Real titles/categories/dates are revealed only when a
passphrase is supplied (they live inside the blob).

### `search`
Decrypts each blob **in memory only**, runs the regex, and discards the
plaintext. One prompt per command. Without a passphrase it skips encrypted notes
and reports how many were skipped. Plaintext never touches disk during search.

### `export`
Archives the ciphertext blobs **as-is, with no passphrase needed**. This is a
feature: it directly removes one of the original exposures — export archives that
used to leak plaintext into synced folders now carry only ciphertext. A plaintext
export, if ever wanted, would be a separate, loud, opt-in `--decrypt` path.

### `obl add` / `obl raise` — the editor flow

The editor needs a file path, so plaintext must be materialized *somewhere*. The
design keeps it off persistent disk and contained:

1. `mkdtemp` a private `0700` working directory in **RAM-backed storage**: prefer
   `$XDG_RUNTIME_DIR`, fall back to `/dev/shm`, and only as a last resort a
   `0600` file in the notes dir **with a printed warning**. (Portability: tmpfs is
   reliable on Linux; macOS lacks it by default, so the fallback chain matters.
   Even tmpfs can reach disk via swap unless swap is encrypted — this is "much
   better," not "perfect.")
2. Decrypt the note into that directory. The editor's own swap/undo/backup files
   are created next to the file it edits, so they land in the tmpfs directory too
   — contained, not scattered into `$HOME` or beside the ciphertext.
3. Run `$EDITOR`. On a **clean exit**, re-encrypt **atomically**: write to a temp
   blob, `fsync`, then `rename()` over the real blob, so a crash never leaves a
   half-written, unreadable note. If the editor exits non-zero, the original is
   left untouched.
4. Overwrite-then-`unlink` the plaintext and remove the temp directory.
5. An `atexit` / signal handler repeats step 4 on `Ctrl-C` mid-edit. Nothing
   survives `SIGKILL` or power loss — a documented, fundamental limit.

## Migration

There is nothing to migrate. Encrypted notes are a new file type alongside `.md`,
so every existing plaintext note is byte-for-byte untouched and parses as before.

Converting is explicit, per-note, and reversible:

- `obl encrypt <id|title>` — read the `.md`, write `<id>.obl`, then
  overwrite-and-unlink the original plaintext (with the recoverability caveat
  noted to the user).
- `obl decrypt <id|title>` — the inverse.

## New surface (gated changes, per AGENTS.md)

This design adds commands and a build-time dependency, both of which AGENTS.md
gates behind a conversation — which issue #15 and this document are.

- New commands: `obl encrypt`, `obl decrypt`.
- `collect_all_notes` becomes encrypted-aware.
- Build: optional `ENABLE_CRYPTO=1` linking libsodium; default build unchanged.

## Open sub-decisions

Small, non-blocking; to settle before implementation:

1. **Blob extension:** `.obl` (current lean) vs `.md.obl` vs an `age`-style name.
2. **`obl add -e` in v1?** Ship create-encrypted now, or start with only
   `encrypt` / `decrypt` on existing notes (current lean: the latter — smaller
   surface).
3. **Locked `search`:** silently skip encrypted notes, or prompt once to include
   them (current lean: prompt once).

## Testing plan

Per AGENTS.md, new features need tests, and tests stay as small POSIX shell
scripts or simple C programs (no framework). Anticipated coverage:

- default build (`make`, no flag) still compiles clean on gcc and clang, and
  encryption commands fail with the "rebuild with `ENABLE_CRYPTO=1`" message;
- round-trip: `add` → `encrypt` → `decrypt` yields byte-identical content;
- `ls` shows encrypted notes as locked without a passphrase, and real metadata
  with one;
- `search` skips encrypted notes when locked and matches them when unlocked,
  never writing plaintext to disk;
- `export` archives ciphertext and the archive contains no plaintext note body;
- wrong passphrase fails cleanly (AEAD authentication), leaving the blob intact;
- the editor flow leaves no plaintext file or editor swap/backup artifact behind
  after a normal edit.

## Out of scope (for now)

- Key files, hardware keys, or multi-recipient encryption.
- A background agent / passphrase caching across invocations.
- Re-keying / passphrase rotation tooling.
- Encrypting note *filenames* beyond using a non-revealing random id (the id is
  already non-revealing, so this is unnecessary).
