# Prophet Branching & Release Workflow

This repo follows a Git-first workflow:

- use classic Git commands for day-to-day branch management,
- use the VS Code GitHub Pull Requests UI for review/merge,
- use `just` mainly for build/test/release helpers.

## 1) Branching strategy

- `main` (protected release branch)
  - Stable, tagged releases only.
  - No direct development commits.
  - Receives release-ready state from `dev`.
- `dev` (active integration branch)
  - Default branch for ongoing engineering work.
  - All feature/fix PRs target `dev`.
- `feature/*` or `fix/*` (short-lived work branches)
  - Branch from `dev`.
  - Merge back to `dev` via PR.

## 2) Daily feature/fix workflow (Git + VS Code PR UI)

1. Update `dev` locally:
   - `git checkout dev`
   - `git pull --ff-only origin dev`
2. Create branch:
   - `git checkout -b feature/<name>` (or `fix/<name>`)
3. Build/test locally while iterating:
   - `just all`
   - `just perft_test` (required for movegen/search changes)
4. Commit and push:
   - `git add -A`
   - `git commit -m "<scope>: <summary>"`
   - `git push -u origin feature/<name>`
5. Open PR in VS Code Pull Requests view:
   - Head: `feature/<name>`
   - Base: `dev`
6. Merge after review and required checks.

## 3) Required checks for `feature/* -> dev`

All merges from feature/fix branches into `dev` must pass perft validation.

- Local requirement before opening/merging PR:
  - `just perft_test`
- Repository policy recommendation:
  - Add a required CI status check for perft on PRs targeting `dev`.
- Local `just` guard:
  - `just finish-feature <name>` runs `just verify-perft` before merge.

> Note: local checks are not a substitute for protected-branch required status checks.

## 4) Release workflow (maintainers)

Releases are performed from a clean working tree.

1. Ensure `dev` is ready and synced.
2. Promote `dev` to `main` (fast-forward policy applies in `just promote-dev-to-main`).
3. Build and archive release artifacts under explicit version directory.
4. Tag and push release tag.

Primary command:

- `just release <x.y.z>`

This now archives all key binaries under `artifacts/v<x.y.z>/`:

- `prophet-release`
- `perft-release`
- `magic_gen-release`

## 5) Artifact/version behavior

- Normal dev builds use `git describe --tags --always --dirty`.
- Release archiving uses explicit version override `v<x.y.z>` to avoid dirty-derived naming.
- UCI version string during release builds is also set to `v<x.y.z>`.

## 6) Fast-forward-only convention

- `git pull --ff-only ...` and `git merge --ff-only ...` are used to prevent accidental merge commits on protected flow branches.
- If branches diverge, promotion fails and maintainers must reconcile branches first.

## 7) Optional helper recipes

Use these as convenience wrappers (not as a replacement for understanding Git):

- `just start-feature <name> [base]`
- `just publish-feature <name>`
- `just finish-feature <name> [delete_local]`
- `just release <ver>`
