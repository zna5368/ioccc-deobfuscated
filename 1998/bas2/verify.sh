#!/usr/bin/env bash
# 
# try.sh - demonstrate IOCCC entry 1998/bas2
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

read -r -n 1 -p "Press any key to run: ./simple < bas2.c: "
echo 1>&2
./simple < bas2.c
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < README.md: "
echo 1>&2
./simple < README.md
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < Makefile: "
echo 1>&2
./simple < Makefile
echo 1>&2

read -r -n 1 -p "Press any key to run: echo foo bar baz | ./simple: "
echo 1>&2
echo foo bar baz | ./simple
echo 1>&2

read -r -n 1 -p "Press any key to run: cat README.md Makefile bas2.c | ./simple: "
echo 1>&2
cat README.md Makefile bas2.c | ./simple
echo 1>&2

# while we have an arg check if it's a file. If it is a regular file that is
# readable feed it to the program; otherwise act as if it's a string.
while [[ "$#" -gt 0 ]]; do
    if [[ -f "$1" && -r "$1" ]]; then
	read -r -n 1 -p "Press any key to run: ./simple < $1: "
	echo 1>&2
	./simple < "$1"
	echo 1>&2
    else
	read -r -n 1 -p "Press any key to run: echo $1 | ./simple: "
	echo 1>&2
	echo "$1" | ./simple
	echo 1>&2
    fi
    shift 1
done
