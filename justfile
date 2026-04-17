set shell := ["sh", "-eu", "-c"]

default: all

# Resolve version from Git metadata (tag/hash), with a fallback for non-git contexts.
version:
    git describe --tags --always --dirty 2>/dev/null || printf '%s\n' dev

# Build all main sources (excluding magic generator and perft test harness).
all build="dev" version_override="": dirs
    cc="${CC:-cc}"; uname_s="$(uname -s)"; uname_m="$(uname -m)"; archflags=""; if [ "$uname_s" = "Darwin" ] && [ "$uname_m" = "arm64" ]; then archflags="-mcpu=native"; fi; case "{{build}}" in debug) mode_cflags="-O0 -g3 -fno-omit-frame-pointer"; mode_ldflags="" ;; release) mode_cflags="-O3 -DNDEBUG -flto"; mode_ldflags="" ;; *) mode_cflags="-O3 -g1 -DNDEBUG"; mode_ldflags="" ;; esac; if [ -n "{{version_override}}" ]; then git_version="{{version_override}}"; else git_version="$(git describe --tags --always --dirty 2>/dev/null || printf '%s' dev)"; fi; sources="$(find src -name '*.c' ! -name 'magic_gen.c' ! -name 'perft_test.c')"; cflags="-std=c17 -Wall -Wextra ${archflags} ${mode_cflags}"; $cc -I src -DVERSION="\"${git_version}\"" $cflags $mode_ldflags $sources -o prophet

run build="dev" version_override="":
    just all "{{build}}" "{{version_override}}"
    ./prophet

# Build and run the magic number generator utility.
magic build="dev" version_override="": dirs
    cc="${CC:-cc}"; uname_s="$(uname -s)"; uname_m="$(uname -m)"; archflags=""; if [ "$uname_s" = "Darwin" ] && [ "$uname_m" = "arm64" ]; then archflags="-mcpu=native"; fi; case "{{build}}" in debug) mode_cflags="-O0 -g3 -fno-omit-frame-pointer"; mode_ldflags="" ;; release) mode_cflags="-O3 -DNDEBUG -flto"; mode_ldflags="" ;; *) mode_cflags="-O3 -g1 -DNDEBUG"; mode_ldflags="" ;; esac; if [ -n "{{version_override}}" ]; then git_version="{{version_override}}"; else git_version="$(git describe --tags --always --dirty 2>/dev/null || printf '%s' dev)"; fi; cflags="-std=c17 -Wall -Wextra ${archflags} ${mode_cflags}"; $cc -I src -DVERSION="\"${git_version}\"" $cflags $mode_ldflags src/attacks/magic_gen.c src/attacks/sliding_attacks.c -o magic_gen
    ./magic_gen

# Build and run perft executable.
perft_test build="dev" version_override="": dirs
    cc="${CC:-cc}"; uname_s="$(uname -s)"; uname_m="$(uname -m)"; archflags=""; if [ "$uname_s" = "Darwin" ] && [ "$uname_m" = "arm64" ]; then archflags="-mcpu=native"; fi; case "{{build}}" in debug) mode_cflags="-O0 -g3 -fno-omit-frame-pointer"; mode_ldflags="" ;; release) mode_cflags="-O3 -DNDEBUG -flto"; mode_ldflags="" ;; *) mode_cflags="-O3 -g1 -DNDEBUG"; mode_ldflags="" ;; esac; if [ -n "{{version_override}}" ]; then git_version="{{version_override}}"; else git_version="$(git describe --tags --always --dirty 2>/dev/null || printf '%s' dev)"; fi; cflags="-std=c17 -Wall -Wextra ${archflags} ${mode_cflags}"; $cc -I src -DVERSION="\"${git_version}\"" $cflags $mode_ldflags src/tests/perft_test.c src/tests/testing_utils.c src/movegen/move.c src/eval/hceval.c src/search/tt.c src/engine/engine.c src/movegen/movegen.c src/movegen/constant_moves.c src/movegen/sliding_moves.c src/movegen/move_make.c src/board/cboard.c src/board/zobrist.c src/utils/prng.c src/attacks/constant_attacks.c src/attacks/sliding_attacks.c -o perft
    ./perft

# Validate perft before merging feature work.
verify-perft:
    just perft_test dev

# Build engine normally and archive a versioned artifact for historical tracking.
archive build="dev" artifact_version="" version_override="":
    just all "{{build}}" "{{version_override}}"
    resolved_version="{{artifact_version}}"; if [ -z "$resolved_version" ]; then resolved_version="$(git describe --tags --always --dirty 2>/dev/null || printf '%s' dev)"; fi; artifact_dir="artifacts/${resolved_version}"; mkdir -p "$artifact_dir"; cp prophet "$artifact_dir/prophet-{{build}}"; printf 'Archived %s\n' "$artifact_dir/prophet-{{build}}"

# Build perft target and archive a versioned artifact for historical tracking.
archive-perft build="dev" artifact_version="" version_override="":
    just perft_test "{{build}}" "{{version_override}}"
    resolved_version="{{artifact_version}}"; if [ -z "$resolved_version" ]; then resolved_version="$(git describe --tags --always --dirty 2>/dev/null || printf '%s' dev)"; fi; artifact_dir="artifacts/${resolved_version}"; mkdir -p "$artifact_dir"; cp perft "$artifact_dir/perft-{{build}}"; printf 'Archived %s\n' "$artifact_dir/perft-{{build}}"

# Build magic generator and archive a versioned artifact for historical tracking.
archive-magic build="dev" artifact_version="" version_override="":
    just magic "{{build}}" "{{version_override}}"
    resolved_version="{{artifact_version}}"; if [ -z "$resolved_version" ]; then resolved_version="$(git describe --tags --always --dirty 2>/dev/null || printf '%s' dev)"; fi; artifact_dir="artifacts/${resolved_version}"; mkdir -p "$artifact_dir"; cp magic_gen "$artifact_dir/magic_gen-{{build}}"; printf 'Archived %s\n' "$artifact_dir/magic_gen-{{build}}"

# Build and archive all release artifacts under one explicit version directory.
archive-release ver:
    just archive release "v{{ver}}" "v{{ver}}"
    just archive-perft release "v{{ver}}" "v{{ver}}"
    just archive-magic release "v{{ver}}" "v{{ver}}"

# List archived artifacts.
list-artifacts:
    find artifacts -type f 2>/dev/null || true

# Create an annotated release tag. Use: just release-tag 0.1.0
release-tag ver:
    git tag -a "v{{ver}}" -m "Version {{ver}}"

# Push release tag to origin. Use: just push-release-tag 0.1.0
push-release-tag ver:
    git push origin "v{{ver}}"

# Print current git branch.
current-branch:
    git rev-parse --abbrev-ref HEAD

# Ensure working tree is clean before branch/release operations.
require-clean:
    git diff --quiet
    git diff --cached --quiet

# Create/switch to the dev branch (based on main if missing).
ensure-dev:
    if git show-ref --verify --quiet refs/heads/dev; then git checkout dev; else git checkout -b dev main; fi

# Publish dev branch to origin and track it.
publish-dev:
    just ensure-dev
    git push -u origin dev

# Start or switch to a feature branch from the base branch (default: dev).
start-feature name base="dev": require-clean
    git checkout "{{base}}"
    git pull --ff-only origin "{{base}}" || true
    if git show-ref --verify --quiet "refs/heads/feature/{{name}}"; then git checkout "feature/{{name}}"; else git checkout -b "feature/{{name}}"; fi

# Publish feature branch to origin and set upstream.
publish-feature name:
    git push -u origin "feature/{{name}}"

# Merge feature branch into dev (no-ff), push dev, and optionally clean up local feature branch.
finish-feature name delete_local="yes": require-clean
    git checkout dev
    git pull --ff-only origin dev || true
    just verify-perft
    git merge --no-ff "feature/{{name}}" -m "Merge feature/{{name}} into dev"
    git push origin dev
    if [ "{{delete_local}}" = "yes" ]; then git branch -d "feature/{{name}}"; fi

# Promote dev -> main (fast-forward only) and push main.
promote-dev-to-main: require-clean
    git checkout main
    git pull --ff-only origin main || true
    git merge --ff-only dev
    git push origin main

# Sync dev with main after a release and push dev.
sync-dev-with-main: require-clean
    git checkout dev
    git pull --ff-only origin dev || true
    git merge --ff-only main
    git push origin dev

# Full release flow from main: ensure clean tree, promote dev, build+archive all release artifacts, create+push tag.
release ver:
    just require-clean
    just promote-dev-to-main
    just archive-release "{{ver}}"
    if git rev-parse -q --verify "refs/tags/v{{ver}}" >/dev/null; then echo "Tag v{{ver}} already exists"; exit 1; fi
    git tag -a "v{{ver}}" -m "Version {{ver}}"
    git push origin "v{{ver}}"
    printf 'Release completed: v%s\n' "{{ver}}"

debug:
    just clean
    just all debug

dev:
    just clean
    just all dev

release-build:
    just clean
    just all release

clean:
    rm -rf build prophet magic_gen perft

dirs:
    mkdir -p build
