

set shell := ["fish", "-c"]

cc := "gcc"
srdir := "src"
builddir := "build"
target := "build/prophet"
magic_target := "magic_gen"

cstd := "-std=c17"
warnflags := "-Wall -Wextra"
cppflags := "-I src"

debug_cflags := "-O0 -g3 -fno-omit-frame-pointer -Wall -Wextra -Wpedantic -Wshadow -flto"
dev_cflags := "-O3 -g1 -DNDEBUG"
release_cflags := "-O3 -DNDEBUG -flto"

debug_ldflags := ""
dev_ldflags := ""
release_ldflags := ""

default:
    @just --list

all build_mode="dev": dirs
    #!/usr/bin/env fish
    set -g fish_trace 1
    set -l cc "{{cc}}"
    set -l build_mode "{{build_mode}}"
    set -l cstd "{{cstd}}"; \
    set -l warnflags "{{warnflags}}"; \
    set -l cppflags "{{cppflags}}"; \
    set -l builddir "{{builddir}}"; \
    set -l srdir "{{srdir}}"; \
    set -l target "{{target}}"; \
    set -l debug_cflags "{{debug_cflags}}"; \
    set -l dev_cflags "{{dev_cflags}}"; \
    set -l release_cflags "{{release_cflags}}"; \
    set -l debug_ldflags "{{debug_ldflags}}"; \
    set -l dev_ldflags "{{dev_ldflags}}"; \
    set -l release_ldflags "{{release_ldflags}}"; \
    set -l archflags ""; \
    if test (uname -s) = "Darwin"; and test (uname -m) = "arm64"; set archflags "-mcpu=native"; end; \
    set -l mode_cflags "$dev_cflags"; \
    set -l mode_ldflags "$dev_ldflags"; \
    if test "$build_mode" = "debug"; set mode_cflags "$debug_cflags"; set mode_ldflags "$debug_ldflags"; end; \
    if test "$build_mode" = "release"; set mode_cflags "$release_cflags"; set mode_ldflags "$release_ldflags"; end; \
    set -l warnflags_list (string split " " -- $warnflags); \
    set -l cppflags_list (string split " " -- $cppflags); \
    set -l mode_cflags_list (string split " " -- $mode_cflags); \
    set -l cflags $cstd $warnflags_list $archflags $mode_cflags_list; \
    set -l ldflags_list (string split " " -- $mode_ldflags); \
    set -l sources (find $srdir -name '*.c' ! -name 'magic_gen.c' ! -name 'perft_test.c'); \
    set -l objects; \
    for src in $sources; \
        set -l rel (string replace -r "^$srdir/" "" -- $src); \
        set -l obj "$builddir/$rel"; \
        set obj (string replace -r '\\.c$' '.o' -- $obj); \
        mkdir -p (dirname $obj); \
        $cc $cppflags_list $cflags -c $src -o $obj; \
        set -a objects $obj; \
    end; \
    mkdir -p (dirname $target); \
    $cc $cppflags_list $cflags $ldflags_list $objects -o $target; \


run: all
    ./{{target}}

perft max_depth="5":
    ./scripts/perft.sh {{target}} {{max_depth}}

debug:
    just all build_mode=debug

dev:
    just all build_mode=dev

release:
    just all build_mode=release

clean:
    rm -rf {{builddir}} {{target}} {{magic_target}}

dirs:
    @mkdir -p {{builddir}}

artifacts_dir := "artifacts"

# Helper: Compiles the release build and copies it to a specific artifact folder
_build-to-artifact folder_name: release
    @mkdir -p {{artifacts_dir}}/{{folder_name}}
    @cp build/prophet {{artifacts_dir}}/{{folder_name}}/prophet

# Helper: Safely builds the main branch without losing your uncommitted work
_build-main:
    @echo "Checking out main to build baseline..."
    @set -l current_branch (git branch --show-current)
    @git checkout main
    @just release
    @mkdir -p {{artifacts_dir}}/main
    @cp build/prophet {{artifacts_dir}}/main/prophet
    @git checkout $current_branch


# Usage: just publish-feature "v1.1.0"
publish-feature version:
    @begin; \
        set -l branch_name (git branch --show-current); \
        if test "$branch_name" = "main"; \
            echo "Error: You must run this from a feature branch, not main."; \
            exit 1; \
        end; \
        \
        echo "\n=== 1. Verifying Perft ==="; \
        just perft || exit 1; \
        \
        echo "\n=== 2. Building Feature Artifact ($branch_name) ==="; \
        just _build-to-artifact $branch_name || exit 1; \
        \
        echo "\n=== 3. Building Main Artifact (Baseline) ==="; \
        just _build-main || exit 1; \
        \
        echo "\n=== 4. Benchmarking Regression Check ==="; \
        echo "Benching main..."; \
        uv run scripts/bench.py --engine "{{artifacts_dir}}/main/prophet" --depth 14 > {{artifacts_dir}}/main/bench.txt; \
        echo "Benching feature..."; \
        uv run scripts/bench.py --engine "{{artifacts_dir}}/$branch_name/prophet" --depth 14 > {{artifacts_dir}}/$branch_name/bench.txt; \
        echo "--- NPS Comparison ---"; \
        grep "Bench NPS" {{artifacts_dir}}/main/bench.txt; \
        grep "Bench NPS" {{artifacts_dir}}/$branch_name/bench.txt; \
        echo "Check the bench above. Press ENTER to proceed to SPRT, or Ctrl+C to abort."; \
        read; \
        \
        echo "\n=== 5. SPRT Verification ==="; \
        cutechess-cli \
            -engine cmd={{artifacts_dir}}/$branch_name/prophet name=$branch_name \
            -engine cmd={{artifacts_dir}}/main/prophet name=main \
            -each proto=uci tc=10+0.1 \
            -openings file=openings.epd format=epd order=random \
            -games 2000 -rounds 1000 -repeat -concurrency 4 \
            -resign movecount=3 score=400 \
            -draw movenumber=34 movecount=8 score=20 \
            -sprt elo0=0 elo1=5 alpha=0.05 beta=0.05 \
            | tee {{artifacts_dir}}/$branch_name/sprt.log; \
        echo "Did the feature pass SPRT (H1 Accepted)? Press ENTER to merge and release, or Ctrl+C to abort."; \
        read; \
        \
        echo "\n=== 6. Merging into Main ==="; \
        git checkout main; \
        git merge $branch_name; \
        \
        echo "\n=== 7. Updating Benchmarks CSV ==="; \
        set -l csv_file "benchmarks.csv"; \
        if not test -f $csv_file; \
            echo "Version,Date,Total Time (s),Total Nodes,Bench NPS" > $csv_file; \
        end; \
        set -l time_val (grep "Total Time" {{artifacts_dir}}/$branch_name/bench.txt | awk '{print $4}'); \
        set -l nodes_val (grep "Total Nodes" {{artifacts_dir}}/$branch_name/bench.txt | awk '{print $4}'); \
        set -l nps_val (grep "Bench NPS" {{artifacts_dir}}/$branch_name/bench.txt | awk '{print $4}'); \
        set -l date_val (date "+%Y-%m-%d"); \
        echo "{{version}},$date_val,$time_val,$nodes_val,$nps_val" >> $csv_file; \
        git add $csv_file; \
        git commit -m "chore: track benchmarks for {{version}}"; \
        \
        echo "\n=== 8. Tagging Release ==="; \
        git tag -a {{version}} -m "Release {{version}} (from $branch_name)"; \
        \
        echo "\n=== 9. Storing Final Release Artifacts ==="; \
        mkdir -p {{artifacts_dir}}/{{version}}; \
        just release; \
        cp build/prophet {{artifacts_dir}}/{{version}}/prophet-{{version}}; \
        cp {{artifacts_dir}}/$branch_name/bench.txt {{artifacts_dir}}/{{version}}/; \
        cp {{artifacts_dir}}/$branch_name/sprt.log {{artifacts_dir}}/{{version}}/; \
        \
        echo "\n=== 10. Pushing to origin ==="; \
        git push origin main --tags; \
        echo "\n🚀 Successfully released {{version}}!"; \
    end