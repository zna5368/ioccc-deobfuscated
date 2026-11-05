#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2015/hou
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


read -r -n 1 -p "Press any key to run: echo IOCCC | ./simple: "
echo 1>&2
echo IOCCC | ./simple
echo 1>&2

read -r -n 1 -p "Press any key to run: head -n 13 rfc1321.txt (space = next page, q = quit): "
echo 1>&2
head rfc1321.txt -n 13 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < rfc1321.txt: "
echo 1>&2
./simple < rfc1321.txt 
echo 1>&2

read -r -n 1 -p "Press any key to run: echo \"\" | ./simple: "
echo 1>&2
echo "" | ./simple
echo 1>&2

read -r -n 1 -p "Press any key to show prog.c (space = next page, q = quit): "
echo 1>&2
less -rEXFK prog.c
echo 1>&2

echo "For the next two, observe the magical values."
echo 1>&2
read -r -n 1 -p "Press any key to run: ./simple < prog.c | cut -c-5: "
echo 1>&2
./simple < prog.c | cut -c-5

read -r -n 1 -p "Press any key to run: ./simple < Makefile | cut -c-4: "
echo 1>&2
./simple < Makefile | cut -c-4
echo 1>&2

read -r -n 1 -p "Press any key to run: wc -c large: "
echo 1>&2
wc -c large
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < large: "
echo 1>&2
./simple < large
