#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2013/dlowe
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

# NOTE: do NOT change this to prompt the user for each command as this will
# break the script diff.sh.
#
echo "$ ./simple 0 1 2 3 4 5 6 7"
./simple 0 1 2 3 4 5 6 7
echo
echo "$ ./simple 16 32 64 128"
./simple 16 32 64 128
echo
echo "$ ./simple 16 32 64 128 256"
./simple 16 32 64 128 256
echo
echo "$ ./simple 16 32 64 128 256 512"
./simple 16 32 64 128 256 512
echo
echo "$ ./simple 0"
./simple 0
echo
echo "$ ./slflen.sh"
./slflen.sh
