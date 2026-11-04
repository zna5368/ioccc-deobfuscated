#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2006/toledo1
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


read -r -n 1 -p "Press any key to run: ./simple 00: "
echo 1>&2
./simple 00
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 30: "
echo 1>&2
./simple 30
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 99: "
echo 1>&2
./simple 99
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 42: "
echo 1>&2
./simple 42
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 11 # start is A8: "
echo 1>&2
./simple 11       # start is A8
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 18 # start is H8: "
echo 1>&2
./simple 18       # start is H8
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 81 # start is A1: "
echo 1>&2
./simple 81       # start is A1
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 88 # start is H1: "
echo 1>&2
./simple 88       # start is H1
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 44 # start is D5: "
echo 1>&2
./simple 44       # start is D5
echo 1>&2

echo "What happened with the last ones?" 1>&2
