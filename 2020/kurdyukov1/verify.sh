#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2020/kurdyukov1
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

# try and find an md5sum tool
MD5SUM="$(type -P md5sum)"
if [[ -z "$MD5SUM" ]]; then
    MD5SUM="$(type -P openssl)"
fi

read -r -n 1 -p "Press any key to run: echo IOCCC | ./simple: "
echo 1>&2
echo IOCCC | ./simple
echo 1>&2

read -r -n 1 -p "Press any key to show prog.c (space = next page, q = quit): "
echo 1>&2
less -rEXFK prog.c
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < prog.c: "
echo 1>&2
./simple < prog.c
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < prog: "
echo 1>&2
./simple < prog
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < prog.x86.asm: "
echo 1>&2
./simple < prog.x86.asm
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < prog.x86_64.asm: "
echo 1>&2
./simple < prog.x86_64.asm
echo 1>&2

# remove output files first
rm -vf md5.txt prog.md5.txt

if [[ -n "$MD5SUM" ]]; then
    if [[ "$(basename "$MD5SUM")" == "openssl" ]]; then
	read -r -n 1 -p "Press any key to run: $MD5SUM md5 prog.c | cut -f 2 -d' ' | tee md5.txt: "
	"$MD5SUM" md5 prog.c | cut -f 2 -d' ' | tee md5.txt
	read -r -n 1 -p "Press any key to run: ./simple < prog.c | tee prog.md5.txt: "
	./simple < prog.c | tee prog.md5.txt
    else
	read -r -n 1 -p "Press any key to run: $MD5SUM prog.c | cut -f1 -d' ' | tee md5.txt: "
	"$MD5SUM" prog.c | cut -f1 -d' ' | tee md5.txt
	read -r -n 1 -p "Press any key to run: ./simple < prog.c > prog.md5.txt: "
	echo 1>&2
	./simple < prog.c > prog.md5.txt
    fi
    echo 1>&2
    read -r -n 1 -p "Press any key to run: diff -s md5.txt prog.md5.txt: "
    echo 1>&2
    diff -s md5.txt prog.md5.txt
    echo 1>&2
fi
rm -vf md5.txt prog.md5.txt
