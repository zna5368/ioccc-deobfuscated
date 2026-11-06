#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2018/yang
#

# make sure CC is set so that when we do make CC="$CC" it isn't empty. Doing it
# this way allows us to have the user specify a different compiler in an easy
# way.
if [[ -z "$CC" ]]; then
    CC="cc"
fi

make CC="$CC" left right shift msg0 msg1 msg2 >/dev/null || exit 1
make CC="$CC" -f /home/stu1/s0/zna5368/SoftDev/ioccc-deobfuscated/.claude/skills/deobfuscate/Makefile simple >/dev/null || exit 1

# clear screen after compilation so that only the entry is shown
clear

read -r -n 1 -p "Press any key to run: ./simple < prog.c (space = next page, q = quit): "
echo 1>&2
./simple < prog.c 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./right < prog.c (space = next page, q = quit): "
echo 1>&2
./right < prog.c 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < prog.c | ./simple (space = next page, q = quit): "
echo 1>&2
./simple < prog.c | ./simple 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./shift < prog.c (space = next page, q = quit): "
echo 1>&2
./shift < prog.c 
echo 1>&2

read -r -n 1 -p "Press any key to run: echo IOCCC | ./simple: "
echo 1>&2
echo IOCCC | ./simple
echo 1>&2
read -r -n 1 -p "Press any key to run: echo IOCCC | ./right: "
echo 1>&2
echo IOCCC | ./right
echo 1>&2
read -r -n 1 -p "Press any key to run: echo IOCCC | ./shift: "
echo 1>&2
echo IOCCC | ./shift
echo 1>&2

read -r -n 1 -p "Press any key to run: ./msg0: "
echo 1>&2
./msg0
echo 1>&2

read -r -n 1 -p "Press any key to run: ./msg1: "
echo 1>&2
./msg1
echo 1>&2

read -r -n 1 -p "Press any key to run: ./msg2: "
echo 1>&2
./msg2

read -r -p "Do you wish to see some of the spoilers? (Y/N)? "
if [[ "$REPLY" == "Y" || "$REPLY" == "y" ]]; then
    echo "$ make spoilers" 1>&2
    make spoilers
    rm -f generated*.c msg*.c
    echo "$ make i_give_up_show_me_the_spoilers"
    make i_give_up_show_me_the_spoilers
    echo 1>&2
    for i in 0 1 2 3 4 5 6 7 8; do
	read -r -n 1 -p "Press any key to run: ./msg$i: "
	echo 1>&2
	./msg"$i"
    done
    echo 1>&2

    read -r -n 1 -p "Press any key to run: ./msg9 < prog.c: "
    echo 1>&2
    ./msg9 < prog.c

    make generated1 1>/dev/null || exit 1
    read -r -n 1 -p "Press any key to run: echo IOCCC | ./generated1: "
    echo 1>&2
    echo IOCCC | ./generated1
fi
