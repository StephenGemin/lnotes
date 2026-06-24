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
- `CLAUDE.md` `## Pull request labels` — when `.github/release.yml` changes,
  update this list to match: add newly-introduced labels and remove deleted ones.

## Pull request labels

Apply at least one label matching the change's primary intent. Pick the dominant
category; add more only when the change genuinely spans several:
`breaking-change`, `feature`, `bug`, `performance`, `refactor`, `security`,
`documentation`, `test`. Use `chore` or `ci` for changelog-excluded work.

This list mirrors `.github/release.yml`, the source of truth. If its labels change,
update this list to match.

## Pull requests

- Builds with `make`.
- Tests pass.
- New features have tests.
- Diff is focused; no formatting churn, no build artifacts, no editor or cache files.
