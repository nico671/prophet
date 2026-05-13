

set shell := ["fish", "-c"]

cc := "gcc"
srdir := "src"
builddir := "build"
target := "outs/prophet/prophet"
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
