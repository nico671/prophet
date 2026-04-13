# Prophet Branching & Release Workflow

This project uses a three-lane branch strategy:

- `main`: release branch (stable, tagged versions only)
- `dev`: integration branch (day-to-day accumulation)
- `feature/*`: short-lived feature branches

The `justfile` automates most branch/release actions.

## Branch model

### `main`

- Contains release-ready code only.
- Receives changes from `dev` through fast-forward promotion.
- Tagged with semantic versions (`v0.1.0`, `v0.2.0`, ...).

### `dev`

- Default integration branch for active work.
- Feature branches are merged into `dev`.
- `dev` is promoted to `main` at release time.

### `feature/*`

- One branch per change.
- Typically branched from `dev`.
- Merged back into `dev` using `--no-ff` for clear history.

## Local automation commands

### Bootstrap / branch setup

- `just ensure-dev`
  - Create `dev` from `main` if missing and switch to it.
- `just publish-dev`
  - Push `dev` to `origin` and set upstream.

### Feature lifecycle

- `just start-feature <name>`
  - Start or switch to `feature/<name>` from `dev`.
- `just publish-feature <name>`
  - Push feature branch and set upstream.
- `just finish-feature <name>`
  - Merge `feature/<name>` into `dev`, push `dev`, and delete local feature branch (default behavior).

Example:

- `just start-feature nmp-improvements`
- `just publish-feature nmp-improvements`
- `just finish-feature nmp-improvements`

### Release lifecycle

- `just release <ver>`
  - Verifies clean tree
  - Promotes `dev` to `main` (fast-forward only)
  - Builds+archives release binary
  - Creates annotated tag `v<ver>`
  - Pushes the tag

Example:

- `just release 0.1.0`

### Build/version helpers

- `just version`
- `just archive <build>`
- `just archive-perft <build>`
- `just archive-magic <build>`
- `just list-artifacts`

## Artifact layout

Artifacts are stored by resolved git version:

- `artifacts/<version>/prophet-<build>`
- `artifacts/<version>/perft-<build>`
- `artifacts/<version>/magic_gen-<build>`

Example:

- `artifacts/v0.1.0/prophet-release`
- `artifacts/v0.1.0-dirty/prophet-dev`

## Versioning behavior

- Build-time version is injected from:
  - `git describe --tags --always --dirty`
- If unavailable, fallback is:
  - `dev`
- UCI advertises:
  - `id name Prophet <version>`

## Recommended pull request flow

1. Branch from `dev` with `just start-feature <name>`
2. Commit and push feature branch
3. Open PR: `feature/<name>` -> `dev`
4. Merge PR into `dev`
5. Periodically release from `dev` to `main` with `just release <ver>`

## Safety expectations

- `just require-clean` is used by workflow recipes to avoid accidental releases/merges with uncommitted work.
- `promote-dev-to-main` uses `--ff-only`, preventing accidental merge commits on `main`.
