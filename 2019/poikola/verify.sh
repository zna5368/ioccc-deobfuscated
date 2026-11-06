#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2019/poikola
#
# 'When you are about to jump, you are all alone and have to make your own
# decisions. Up there, it's all "up yours".'
#
# -- Matti Ensio Nykänen
#

# make sure CC is set so that when we do make CC="$CC" it isn't empty. Doing it
# this way allows us to have the user specify a different compiler in an easy
# way.
if [[ -z "$CC" ]]; then
    CC="cc"
fi

rm -f simple; make -f ../../.claude/skills/deobfuscate/Makefile CC="$CC" simple >/dev/null || exit 1

# clear screen after compilation so that only the entry is shown
clear

read -r -n 1 -p "Press any key to run: ./simple 512 prog.c: "
echo 1>&2
./simple 512 prog.c
echo 1>&2

echo "$ TZ=UTC24 make CC=$CC simple" 1>&2
rm -f simple; TZ=UTC24 make -f ../../.claude/skills/deobfuscate/Makefile CC="$CC" simple >/dev/null || exit 1
read -r -n 1 -p "Press any key to run: ./simple 512 prog.c: "
echo 1>&2
./simple 512 prog.c
echo 1>&2

echo "$ TZ=UTC48 make CC=$CC simple" 1>&2
rm -f simple; TZ=UTC48 make -f ../../.claude/skills/deobfuscate/Makefile CC="$CC" simple >/dev/null || exit 1
read -r -n 1 -p "Press any key to run: ./simple 512 prog.c: "
echo 1>&2
./simple 512 prog.c
echo 1>&2
