#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2015/burton
#

# make sure CC is set so that when we do make CC="$CC" it isn't empty. Doing it
# this way allows us to have the user specify a different compiler in an easy
# way.
if [[ -z "$CC" ]]; then
    CC="cc"
fi

make CC="$CC" all >/dev/null || exit 1
cp -f simple esimple

# clear screen after compilation so that only the entry is shown
clear

read -r -n 1 -p "Press any key to run: ./simple '?0x3343<<2': "
echo 1>&2
./simple '?0x3343<<2'
echo 1>&2

read -r -n 1 -p "Press any key to run: echo '?0x3343<<2' | ./esimple: "
echo 1>&2
echo '?0x3343<<2' | ./esimple
echo 1>&2


read -r -n 1 -p "Press any key to run: grep '[0-9]' prog.c || echo \"no digits in prog.c!\": "
echo 1>&2
grep -q '[0-9]' prog.c || echo "no digits found in prog.c!"
echo 1>&2

read -r -n 1 -p "Press any key to show calc.in (space = next page, q = quit): "
echo 1>&2
less -rEXFK test.in
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < test.in (space = next page, q = quit): "
echo 1>&2
./simple < test.in 
echo 1>&2

read -r -n 1 -p "Press any key to show try.this.txt (space = next page, q = quit): "
echo 1>&2
less -rEXFK try.this.txt
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < try.this.txt (space = net page, q = quit): "
echo 1>&2
./simple < try.this.txt 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./esimple < try.this.txt (space = next page, q = quit): "
echo 1>&2
./esimple < try.this.txt 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 42 # show hex value of 42: "
echo 1>&2
./simple 42
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple '1<<22-1' # show mask value for 22 bits: "
echo 1>&2
./simple '1<<22-1'
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple ?0x11223344 # byte-swapped value of 0x11223344: "
echo 1>&2
./simple '?0x11223344'
echo 1>&2
rm -f esimple
