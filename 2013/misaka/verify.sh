#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2013/misaka
#

# make sure CC is set so that when we do make CC="$CC" it isn't empty. Doing it
# this way allows us to have the user specify a different compiler in an easy
# way.
if [[ -z "$CC" ]]; then
    CC="cc"
fi

# simple.c is one program, so each tiled build of the original is simple.c
# compiled with a different number of copies.
bin=$(mktemp -d) || exit 1
trap 'rm -rf "$bin"' EXIT
build() {
    "$CC" -std=gnu11 -O3 -DCOPIES_WIDE="$2" -DCOPIES_HIGH="$3" simple.c -o "$bin/$1" || exit 1
}
build horizontal_cat 1 1
build long_cat 1 2
build loong_cat 1 3
build loooong_cat 1 4
build loooooooong_cat 1 5
build long_fat_cat 2 2
build long_faat_cat 3 2
build same_as_long_cat 1 2

# clear screen after compilation so that only the entry is shown
clear

read -r -n 1 -p "Press any key to run: seq 1 12 | ./horizontal_cat - - -: "
echo 1>&2
seq 1 12 | "$bin/horizontal_cat" - - -
echo 1>&2

read -r -n 1 -p "Press any key to run: ./long_cat: "
echo 1>&2
"$bin/long_cat"
echo 1>&2

rm -f long_cat.txt same_as_long_cat.txt
read -r -n 1 -p "Press any key to run: ./long_cat | tee long_cat.txt: "
echo 1>&2
"$bin/long_cat" | tee long_cat.txt
echo 1>&2
read -r -n 1 -p "Press any key to run: ./same_as_long_cat | tee same_as_long_cat.txt: "
echo 1>&2
"$bin/same_as_long_cat" | tee same_as_long_cat.txt
echo 1>&2

read -r -n 1 -p "Press any key to run: diff -s long_cat.txt same_as_long_cat.txt: "
echo 1>&2
diff -s long_cat.txt same_as_long_cat.txt
echo 1>&2

read -r -n 1 -p "Press any key to run: ./long_faat_cat: "
echo 1>&2
"$bin/long_faat_cat"
echo 1>&2

read -r -n 1 -p "Press any key to run: ./long_fat_cat: "
echo 1>&2
"$bin/long_fat_cat"
echo 1>&2

read -r -n 1 -p "Press any key to run: ./loong_cat: "
echo 1>&2
"$bin/loong_cat"
echo 1>&2

read -r -n 1 -p "Press any key to run: ./loooong_cat: "
echo 1>&2
"$bin/loooong_cat"
echo 1>&2

read -r -n 1 -p "Press any key to run: ./loooooooong_cat (space = next page, q = quit): "
echo 1>&2
"$bin/loooooooong_cat" 
echo 1>&2

echo "$ seq -f '%.0f  ' 45 | ./horizontal_cat - misaka.c"
read -r -n 1 -p "Press any key to continue (space = next page, q = quit): "
echo 1>&2
seq -f '%.0f  ' 45 | "$bin/horizontal_cat" - misaka.c 
echo 1>&2
