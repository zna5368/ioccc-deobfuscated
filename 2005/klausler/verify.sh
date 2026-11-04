#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2005/klausler
#

# make sure CC is set so that when we do make CC="$CC" it isn't empty. Doing it
# this way allows us to have the user specify a different compiler in an easy
# way.
if [[ -z "$CC" ]]; then
    CC="cc"
fi

make CC="$CC" all >/dev/null || exit 1

# clear screen after compilation so that only the entry is shown
clear

read -r -n 1 -p "Press any key to run: ./simple peter klausler | head -n 10: "
echo 1>&2
./simple peter klausler | head -n 10
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple abc def ghi jkl mno | head -n 10: "
echo 1>&2
./simple abc def ghi jkl mno | head -n 10
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple foo bar baz | head -n 10: "
echo 1>&2
./simple foo bar baz | head -n 10
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple zebra elephant bear fox | head -n 10: "
echo 1>&2
./simple zebra elephant bear fox | head -n 10
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple the lion king | head -n 10: "
echo 1>&2
./simple the lion king | head -n 10
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple fire breathing dragon | head -n 10: "
echo 1>&2
./simple fire breathing dragon | head -n 10
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple lord of the rings | head -n 10: "
echo 1>&2
./simple lord of the rings | head -n 10
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple fawlty towers | head -n 10: "
echo 1>&2
./simple fawlty towers | head -n 10
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple carrion fowl | head -n 10: "
echo 1>&2
./simple carrion fowl | head -n 10
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple werewolf vampire dragon | head -n 10: "
echo 1>&2
./simple werewolf vampire dragon | head -n 10
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple werewolf vampire elf dragon | head -n 10: "
echo 1>&2
./simple werewolf vampire elf dragon | head -n 10
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple werewolf vampire | head -n 10: "
echo 1>&2
./simple werewolf vampire | head -n 10
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple dennis ritchie | head -n 10: "
echo 1>&2
./simple dennis ritchie | head -n 10
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple ken thompson | head -n 10: "
echo 1>&2
./simple ken thompson | head -n 10
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple Linus Torvalds | head -n 10: "
echo 1>&2
./simple linus torvalds | head -n 10
echo 1>&2
