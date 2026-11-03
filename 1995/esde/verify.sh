#!/usr/bin/env bash
#
# try.sh - show IOCCC entry 1995/esde
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

read -r -n 1 -p "Press any key to show esde.data: "
echo 1>&2
cat esde.data
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple esde.data Johanson: "
echo 1>&2
./simple esde.data Johanson
echo 1>&2

read -r -n 1 -p "Press any key to show esde.data2: "
echo 1>&2
cat esde.data2
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple esde.data2 read: "
echo 1>&2
./simple esde.data2 read
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple README.md date: "
echo 1>&2
./simple README.md date
echo 1>&2
