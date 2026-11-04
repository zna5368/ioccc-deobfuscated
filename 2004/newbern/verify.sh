#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2004/newbern
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

read -r -n 1 -p "Press any key to run: ./simple newbern.dat \"IOCCC 2004\" (space = next page, q = quit): "
echo 1>&2
./simple newbern.dat "IOCCC 2004" 
echo 1>&2

read -r -n 1 -p "Press any key to run: echo SOS | ./simple: "
echo 1>&2
echo SOS | ./simple
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple newbern.dat IOCCC x (space = next page, q = quit): "
echo 1>&2
./simple newbern.dat IOCCC x 
echo 1>&2
