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
