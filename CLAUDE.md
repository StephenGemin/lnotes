# oubliette

@AGENTS.md

## Documentation

Never update documentation without explicit approval. When documentation changes are
warranted, propose a summary of the changes and wait for confirmation before editing
any file.

Files to keep in sync:
- `README.md` — update when a new feature is added or CLI behavior changes.
- `CHANGELOG` — add an entry for new features, bug fixes, refactors, and breaking changes.
- `AGENTS.md` `## Project structure` section — update when the directory layout changes.

## Pull requests

- Builds with `make`.
- Tests pass.
- New features have tests.
- Diff is focused; no formatting churn, no build artifacts, no editor or cache files.
- Apply at least one label that matches the change's primary intent, drawn from the
  labels defined in `.github/release.yml`: `breaking-change`, `feature`, `bug`,
  `performance`, `refactor`, `security`, `documentation`, `test`, or `chore`/`ci`
  for changelog-excluded work. Pick the dominant category; add more labels only when
  the change genuinely spans several. Treat `.github/release.yml` as the source of
  truth — if its labels change, follow that file rather than this list.
