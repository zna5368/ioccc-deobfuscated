#!/usr/bin/env bash
# 
# try.sh - demonstrate IOCCC entry 1998/dorssel
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

read -r -n 1 -p "Press any key to run: echo 'Simple as 123' | ./simple: "
echo 1>&2
echo 'Simple as 123' | ./simple
echo 1>&2

read -r -n 1 -p "Press any key to run: echo 'Binary as simple as 1, 10 11.' | ./simple: " 1>&2
echo 1>&2
echo 'Binary as simple as 1, 10 11.' | ./simple
echo 1>&2

echo "$ echo '.... --- .--  -.. --- . ...  .. -  -.. ---  - .... .- -' | ./simple" 1>&2
read -r -n 1 -p "Press any key to continue: "
echo 1>&2
echo '.... --- .--  -.. --- . ...  .. -  -.. ---  - .... .- -' | ./simple
echo 1>&2

read -r -n 1 -p "Press any key to run: echo 'HOW DOES IT DO THAT' | ./simple: "
echo 1>&2
echo 'HOW DOES IT DO THAT' | ./simple
echo 1>&2

read -r -n 1 -p "Press any key to run: echo 'HOW DOES IT DO THAT' | ./simple | ./simple: "
echo 1>&2
echo 'HOW DOES IT DO THAT' | ./simple | ./simple
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < dorssel.c | ./simple: "
echo 1>&2
./simple < dorssel.c | ./simple
echo 1>&2


echo "$ echo '
..  - .... .. -. -.-  - .... .. ...  ... .--. --- .. .-.. . .-.  .. ...  -- --- .-. .  --- -... ..-. ..- ... -.-. .- - . -..  - .... .- -.  - .... .  .--. .-. --- --. .-. .- --

. -..- . .-. -.-. .. ... .
-.-. .... .- -. --. .  - .... .  .--. .-. --- --. .-. .- --  ... ---  .. -  -.. --- . ...  -. --- -  ... .... --- ..- -' | ./simple" 1>&2
read -r -n 1 -p "Press any key to continue: "
echo 1>&2

echo "
..  - .... .. -. -.-  - .... .. ...  ... .--. --- .. .-.. . .-.  .. ...  -- --- .-. .  --- -... ..-. ..- ... -.-. .- - . -..  - .... .- -.  - .... .  .--. .-. --- --. .-. .- --

. -..- . .-. -.-. .. ... .
-.-. .... .- -. --. .  - .... .  .--. .-. --- --. .-. .- --  ... ---  .. -  -.. --- . ...  -. --- -  ... .... --- ..- -" | ./simple

read -r -n 1 -p "Press any key to run: ./simple < dorssel.md (space = next page, q = quit): "
echo 1>&2
./simple < dorssel.md 
echo 1>&2
