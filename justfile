cc := "gcc"
builddir := "build"

# 1. Grab the branch globally using backticks so `just` knows it immediately
branch := `git branch --show-current || echo detached`

cstd := "-std=c17"
warnflags := "-Wall -Wextra -Wpedantic -Wshadow"
cppflags := "-I src"

# Recursively find all C files and replace newlines with spaces
sources := `find src -name "*.c" ! -name "magic_gen.c" ! -name "perft_test.c" | tr '\n' ' '`

debug_cflags   := "-O0 -g3 -fno-omit-frame-pointer"
dev_cflags     := "-O3 -g1 -DNDEBUG"
release_cflags := "-O3 -DNDEBUG -flto"

default:
    @just --list

build build_mode="dev" output="" strict="0":
    #!/usr/bin/env bash
    set -euo pipefail
    target="{{output}}"
    if [[ -z "$target" ]]; then target="artifacts/{{branch}}/prophet-{{build_mode}}"; fi
    mkdir -p "$(dirname "$target")"
    flags="{{dev_cflags}}"
    if [[ "{{build_mode}}" == "debug" ]]; then flags="{{debug_cflags}}"; fi
    if [[ "{{build_mode}}" == "release" ]]; then flags="{{release_cflags}}"; fi
    if [[ "{{build_mode}}" == "sanitize" ]]; then flags="{{debug_cflags}} -fsanitize=address,undefined"; fi
    if [[ "{{strict}}" == "1" ]]; then flags="$flags -Werror"; fi
    echo "Building [{{build_mode}}] at $target..."
    {{cc}} {{cstd}} {{warnflags}} {{cppflags}} $flags {{sources}} -o "$target"

run build_mode="dev":
    @echo "Running chess engine in [{{build_mode}}] mode for branch [{{branch}}]..."
    @artifacts/{{branch}}/prophet-{{build_mode}}

clean:
    rm -rf {{builddir}} artifacts/{{branch}}/prophet-*

# Run perft testing using the engine's standard output
perft build_mode="dev":
    #!/usr/bin/env bash
    just build {{build_mode}}
    set -euo pipefail
    
    target="artifacts/{{branch}}/prophet-{{build_mode}}"
    python3 scripts/strict_perft.py --engine "$target"

bench-local depth="7" build_mode="dev":
    #!/usr/bin/env bash
    just build {{build_mode}}
    set -euo pipefail

    target="artifacts/{{branch}}/prophet-{{build_mode}}"
    echo "Running benchmark at depth {{depth}} using engine binary at $target..."

    printf 'uci\nisready\nbench {{depth}}\nquit\n' | "$target"

baseline:
    @python3 -u scripts/validation.py baseline

check:
    #!/usr/bin/env bash
    set -euo pipefail
    python3 -u scripts/check_config.py
    mkdir -p validation-runs
    run_dir="$(mktemp -d validation-runs/check.XXXXXX)"
    target="$run_dir/prophet-dev"
    just build dev "$target" 1
    python3 -u scripts/strict_perft.py --engine "$target"
    python3 -u scripts/uci_smoke.py --engine "$target"
    python3 -u scripts/draw_rules_smoke.py --engine "$target"
    target="$run_dir/prophet-sanitize"
    just build sanitize "$target" 1
    ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 python3 -u scripts/uci_smoke.py --engine "$target"
    ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 python3 -u scripts/draw_rules_smoke.py --engine "$target"

bench baseline="":
    @python3 -u scripts/validation.py benchmark {{ if baseline == "" { "" } else { "--baseline " + baseline } }}

sprt baseline="":
    @python3 -u scripts/validation.py sprt {{ if baseline == "" { "" } else { "--baseline " + baseline } }}

validate version speed_override="0":
    @python3 -u scripts/validation.py validate "{{version}}" {{ if speed_override == "1" { "--speed-override" } else { "" } }}

release version manifest="" speed_override="0":
    @python3 -u scripts/validation.py release "{{version}}" {{ if manifest == "" { "" } else { "--manifest " + manifest } }} {{ if speed_override == "1" { "--speed-override" } else { "" } }}

bootstrap-validation:
    @python3 -u scripts/validation.py bootstrap

format:
    @find src/ \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +
