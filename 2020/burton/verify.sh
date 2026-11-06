#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2020/burton
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

read -r -n 1 -p "Press any key to run: make test: "
echo 1>&2
make test
echo 1>&2

read -r -n 1 -p "Press any key to run: ./check_be.sh (Big Endian) (space = next page, q = quit): "
echo 1>&2
./check_be.sh 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./check_le.sh (Little Endian) (space = next page, q = quit): "
echo 1>&2
./check_le.sh 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 128: "
echo 1>&2
./simple 128
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple_le 128: "
echo 1>&2
./simple_le 128
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple_be 128: "
echo 1>&2
./simple_be 128
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 170: "
echo 1>&2
./simple 170
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 85: "
echo 1>&2
./simple 85
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 128 128: "
echo 1>&2
./simple 128 128
echo 1>&2
echo "Why did that print strange output?"
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 128 128 128: "
echo 1>&2
./simple 128 128 128
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 128 128 128 128: "
echo 1>&2
./simple 128 128 128 128
echo 1>&2
echo "Why did that print strange output?"
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 1; ./simple_be 1: "
echo 1>&2
./simple 1; ./simple_be 1
echo 1>&2
