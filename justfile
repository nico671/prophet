

set shell := ["fish", "-c"]

cc := "gcc"
srdir := "src"
builddir := "build"
target := "prophet"
magic_target := "magic_gen"
perft_target := "perft"

cstd := "-std=c17"
warnflags := "-Wall -Wextra"
cppflags := "-I src"

debug_cflags := "-O0 -g3 -fno-omit-frame-pointer -Wall -Wextra -Wpedantic -Wshadow -O3 -flto"
dev_cflags := "-O3 -g1 -DNDEBUG"
release_cflags := "-O3 -DNDEBUG -flto"

debug_ldflags := ""
dev_ldflags := ""
release_ldflags := ""

default: all

all build_mode="dev": dirs
    @begin; \
        set -l cc "{{cc}}"; \
        set -l build_mode "{{build_mode}}"; \
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
        $cc $cppflags_list $cflags $ldflags_list $objects -o $target; \
    end

run: all
    ./{{target}}

perft build_mode="dev": dirs
    @begin; \
        set -l cc "{{cc}}"; \
        set -l build_mode "{{build_mode}}"; \
        set -l cstd "{{cstd}}"; \
        set -l warnflags "{{warnflags}}"; \
        set -l cppflags "{{cppflags}}"; \
        set -l builddir "{{builddir}}"; \
        set -l srdir "{{srdir}}"; \
        set -l perft_target "{{perft_target}}"; \
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
        set -l sources \
            "{{srdir}}/tests/perft_test.c" \
            "{{srdir}}/tests/testing_utils.c" \
            "{{srdir}}/movegen/move.c" \
            "{{srdir}}/eval/hceval.c" \
            "{{srdir}}/search/search.c" \
            "{{srdir}}/search/tt.c" \
            "{{srdir}}/engine/engine.c" \
            "{{srdir}}/movegen/movegen.c" \
            "{{srdir}}/movegen/constant_moves.c" \
            "{{srdir}}/movegen/sliding_moves.c" \
            "{{srdir}}/movegen/move_make.c" \
            "{{srdir}}/board/cboard.c" \
            "{{srdir}}/board/zobrist.c" \
            "{{srdir}}/utils/prng.c" \
            "{{srdir}}/attacks/constant_attacks.c" \
            "{{srdir}}/attacks/sliding_attacks.c"; \
        set -l objects; \
        for src in $sources; \
            set -l rel (string replace -r "^$srdir/" "" -- $src); \
            set -l obj "$builddir/$rel"; \
            set obj (string replace -r '\\.c$' '.o' -- $obj); \
            mkdir -p (dirname $obj); \
            $cc $cppflags_list $cflags -c $src -o $obj; \
            set -a objects $obj; \
        end; \
        $cc $cppflags_list $cflags $ldflags_list $objects -o $perft_target; \
        ./$perft_target; \
    end

debug:
    @just all build_mode=debug

dev:
    @just all build_mode=dev

release:
    @just all build_mode=release

clean:
    rm -rf {{builddir}} {{target}} {{magic_target}} {{perft_target}}

dirs:
    @mkdir -p {{builddir}}

