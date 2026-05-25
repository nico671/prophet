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
    # 2. Use the {{branch}} variable to make your directory
    @mkdir -p "artifacts/{{branch}}"
    @echo "Building chess engine in [{{build_mode}}] mode for branch [{{branch}}]..."
    
    # 3. Construct the output path directly in the string concatenation
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
    set -euo pipefail
    
    target="artifacts/{{branch}}/prophet-{{build_mode}}"
    echo "Running perft tests using engine binary at $target..."
    if [[ ! -x "$target" ]]; then
        echo "Error: Engine binary not found at $target"
        echo "Run 'just build {{build_mode}}' first."
        exit 1
    fi
    
    printf 'uci\nisready\nperft suite\nquit\n' | "$target"

# Run NNUE training data generation
gendata num_games start_index='0' build_mode="dev" depth='6' temp='50' max_plies='512' adjudicate_cp='1200' adjudicate_plies='6' output="training_data.bin":
    #!/usr/bin/env bash
    set -euo pipefail

    just build {{build_mode}}

    target="artifacts/{{branch}}/prophet-{{build_mode}}"
    echo "Generating data using engine binary at $target..."
    if [[ ! -x "$target" ]]; then
        echo "Error: Engine binary not found at $target"
        echo "Run 'just build {{build_mode}}' first."
        exit 1
    fi

    "$target" --gendata {{num_games}} \
        --start-index {{start_index}} \
        --depth {{depth}} \
        --temp {{temp}} \
        --max-plies {{max_plies}} \
        --adjudicate-cp {{adjudicate_cp}} \
        --adjudicate-plies {{adjudicate_plies}} \
        --output {{output}}
