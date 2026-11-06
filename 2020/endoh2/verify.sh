#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2020/endoh2
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

read -r -n 1 -p "Press any key to run: ./simple: "
echo 1>&2
./simple
echo 1>&2

read -r -n 1 -p "Press any key to show hello.txt: "
echo 1>&2
cat hello.txt
echo 1>&2
read -r -n 1 -p "Press any key to run: ./simple hello.txt: "
echo 1>&2
./simple hello.txt
echo 1>&2

read -r -n 1 -p "Press any key to show yoda.txt: "
echo 1>&2
cat yoda.txt
echo 1>&2
read -r -n 1 -p "Press any key to run: ./simple yoda.txt: "
echo 1>&2
./simple yoda.txt

read -r -n 1 -p "Press any key to show bear.txt: "
echo 1>&2
cat bear.txt
read -r -n 1 -p "Press any key to run: ./simple bear.txt: "
echo 1>&2
./simple bear.txt

