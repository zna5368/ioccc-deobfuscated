#!/usr/bin/env bash
#
# simple.sh - driver for the deobfuscated 1989/fubar (simple.c)
#
# The original source was both a shell script and a C program, so it could
# compile and run itself. Here the shell half lives in this script instead.
#
# Each pass compiles the current generation of the source, feeds it its own
# source, and replaces the source with the next generation it prints. The
# program exits non-zero once it has printed the factorial.

if [[ $# -ne 1 ]]; then
    echo "usage: $0 number" 1>&2
    exit 1
fi

rm -f ouroboros.c x1 x
cp simple.c ouroboros.c

while true; do
    "${CC:-cc}" ouroboros.c -o x || exit 1
    if ! ./x "$1" < ouroboros.c > x1; then
        exit 0
    fi
    mv x1 ouroboros.c
done
