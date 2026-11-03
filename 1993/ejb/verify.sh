#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 1993/ejb
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

read -r -n 1 -p "Press any key to play the patience puzzle: ./simple -3: "
echo 1>&2
./simple -3
echo 1>&2

read -r -n 1 -p "Press any key to play the Tower of Hanoi puzzle: ./simple 3: "
echo 1>&2
./simple 3
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 0: "
echo 1>&2
./simple 0
exit 0
