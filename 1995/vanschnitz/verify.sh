#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 1995/vanschnitz
#

# make sure CC is set so that when we do make CC="$CC" it isn't empty. Doing it
# this way allows us to have the user specify a different compiler in an easy
# way.
if [[ -z "$CC" ]]; then
    CC="cc"
fi

make CC="$CC" all >/dev/null || exit 1
rm -f simple; make -f /workspaces/ioccc-deobfuscated/.claude/skills/deobfuscate/Makefile CC="$CC" simple >/dev/null 2>&1 || exit 1

# clear screen after compilation so that only the entry is shown
clear

read -r -n 1 -p "Press any key to run: ./simple (space = next page, q = quit): "
echo 1>&2
./simple 
echo 1>&2

for i in $(seq 1 6); do
    read -r -n 1 -p "Press any key to set level at $i (make clobber all LEVEL=$i): "
    echo 1>&2
    make clobber all LEVEL="$i"
    rm -f simple; make -f /workspaces/ioccc-deobfuscated/.claude/skills/deobfuscate/Makefile simple LEVEL="$i" >/dev/null 2>&1
    read -r -n 1 -p "Press any key to run: ./simple (space = next page, q = quit): "
    echo 1>&2
    ./simple 
done
