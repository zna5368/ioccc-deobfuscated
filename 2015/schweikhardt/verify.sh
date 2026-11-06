#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2015/schweikhardt
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

echo "$ ./simple 19" 1>&2
./simple 19

read -r -n 1 -p "Press any key to run: ./simple 2410 (space = next page, q = quit): "
echo 1>&2
./simple 2410 
read -r -n 1 -p "Press any key to run: ./simple abfff (space = next page, q = quit): "
echo 1>&2
./simple abfff 
read -r -n 1 -p "Press any key to run: ./simple 27f8cebf (space = next page, q = quit): "
echo 1>&2
./simple 27f8cebf 
read -r -n 1 -p "Press any key to run: ./simple 246f8fddf (space = next page, q = quit): "
echo 1>&2
./simple 246f8fddf 
read -r -n 1 -p "Press any key to run: ./simple 2e95ab51ffea9de (space = next page, q = quit): "
echo 1>&2
./simple 2e95ab51ffea9de 
read -r -n 1 -p "Press any key to run: ./simple e6a5c22fd7bde9f (space = next page, q = quit): "
echo 1>&2
./simple e6a5c22fd7bde9f 
read -r -n 1 -p "Press any key to run: ./simple 1b7dd73a937485bf (space = next page, q = quit): "
echo 1>&2
./simple 1b7dd73a937485bf 
read -r -n 1 -p "Press any key to run: ./simple 7d3237680d190a77e53751b (space = next page, q = quit): "
echo 1>&2
./simple 7d3237680d190a77e53751b 
read -r -n 1 -p "Press any key to run: ./simple 302ab3d052fb87c06228d249581be0e4 (space = next page, q = quit): "
echo 1>&2
./simple 302ab3d052fb87c06228d249581be0e4 
echo 1>&2
read -r -n 1 -p "Press any key to run ./simple with a googol (10^100) (space = next page, q = quit): "
echo 1>&2
./simple 1249AD2594C37CEB0B2784C4CE0BF38ACE408E211A7CAAB24308A82E8F10000000000000000000000000 
