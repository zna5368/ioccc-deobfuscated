#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 1984/laman
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

run()
{
    if [[ "$#" -ne 1 ]]; then
	echo "$0: expected 1 arg, got: $#" 1>&2
	return
    fi

    read -r -n 1 -p "Press any key to run: ./simple $1: "
    echo 1>&2
    ./simple "$1"
    echo 1>&2
}


run_twice()
{
    if [[ "$#" -ne 1 ]]; then
	echo "$0: expected 1 arg, got: $#" 1>&2
	return
    fi

    read -r -n 1 -p "Press any key to run: ./simple $1; ./simple $1: "
    echo 1>&2
    ./simple "$1"; ./simple "$1"
    echo 1>&2
}
run 4
run 9
run 16

run_twice 4
run_twice 9
run_twice 16

read -r -n 1 -p "Press any key to run: ./simple 4; ./simple 9; ./simple 16: "
echo 1>&2
./simple 4; ./simple 9; ./simple 16
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 4; ./simple 8; ./simple 16; ./simple 32; ./simple 64: "
echo 1>&2
./simple 4; ./simple 8; ./simple 16; ./simple 32; ./simple 64
echo 1>&2
