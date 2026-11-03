#!/usr/bin/env bash
#
# try.sh - show IOCCC entry 1996/rcm
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


read -r -n 1 -p "Press any key to run: ./simple < rfc1951.gz (space = next page, q = quit): "
echo 1>&2
./simple < rfc1951.gz 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < rfc1952.gz (space = next page, q = quit): "
echo 1>&2
./simple < rfc1952.gz 
echo 1>&2

read -r -n 1 -p "Press any key to run: cp -vf rcm.c rcm.cp.c; gzip rcm.cp.c: "
echo 1>&2
cp -vf rcm.c rcm.cp.c; gzip rcm.cp.c

read -r -n 1 -p "Press any key to run: ./simple < rcm.cp.c.gz | tee rcm.cp.c (space = next page, q = quit): "
echo 1>&2
./simple < rcm.cp.c.gz | tee rcm.cp.c 
echo 1>&2
rm rcm.cp.c.gz

read -r -n 1 -p "Press any key to run: gzip -c < rcm.c | ./simple | tee rcm.cp2.c (space = next page, q = quit): "
echo 1>&2
gzip -c < rcm.c | ./simple | tee rcm.cp2.c 
echo 1>&2

read -r -n 1 -p "Press any key to show diff between rcm.cp.c and rcm.cp2.c: "
echo 1>&2
diff -s rcm.cp.c rcm.cp2.c
echo 1>&2
rm -f rcm.cp.c rcm.cp2.c

