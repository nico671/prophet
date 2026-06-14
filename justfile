cc := "gcc"
builddir := "build"

# 1. Grab the branch globally using backticks so `just` knows it immediately
branch := `git branch --show-current`

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

build build_mode="dev":
    @mkdir -p "artifacts/{{branch}}"
    @echo "Building chess engine in [{{build_mode}}] mode for branch [{{branch}}]..."
    
    {{ if build_mode == "debug" { \
        cc + " " + cstd + " " + warnflags + " " + cppflags + " " + debug_cflags + " " + sources + " -o artifacts/" + branch + "/prophet-" + build_mode \
    } else if build_mode == "release" { \
        cc + " " + cstd + " " + warnflags + " " + cppflags + " " + release_cflags + " " + sources + " -o artifacts/" + branch + "/prophet-" + build_mode \
    } else { \
        cc + " " + cstd + " " + warnflags + " " + cppflags + " " + dev_cflags + " " + sources + " -o artifacts/" + branch + "/prophet-" + build_mode \
    } }}

clean:
    rm -rf {{builddir}} artifacts/{{branch}}/prophet-*

# Run perft testing using the engine's standard output
perft build_mode="dev":
    #!/usr/bin/env bash
    just build {{build_mode}}
    set -euo pipefail
    
    target="artifacts/{{branch}}/prophet-{{build_mode}}"
    echo "Running perft tests using engine binary at $target..."
    
    printf 'uci\nisready\nperft suite\nquit\n' | "$target"

format:
    @find src/ \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +