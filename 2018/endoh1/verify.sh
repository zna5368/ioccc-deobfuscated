#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2018/endoh1
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

read -r -n 1 -p "Press any key to show prog.c (space = next page, q = quit): "
echo 1>&2
less -rEXFK prog.c
echo "$ ./simple < prog.c > prog.gif" 1>&2
./simple < prog.c > prog.gif
echo 1>&2

read -r -n 1 -p "Press any key to show golem.txt (space = next page, q = quit): "
echo 1>&2
less -rEXFK golem.txt
echo "$ ./simple < golem.txt > golem.gif" 1>&2
./simple < golem.txt > golem.gif
echo 1>&2

read -r -n 1 -p "Press any key to show smily.txt (space = next page, q = quit): "
echo 1>&2
less -rEXFK smily.txt
echo "$ ./simple < smily.txt > smily.gif" 1>&2
./simple < smily.txt > smily.gif
echo 1>&2

read -r -n 1 -p "Press any key to show invisible.txt (space = next page, q = quit): "
less -rEXFK invisible.txt
echo "$ ./simple < invisible.txt > invisible.gif" 1>&2
./simple < invisible.txt > invisible.gif
echo 1>&2

echo "Now open the GIF files prog.gif, golem.gif, smily.gif and invisible.gif" 1>&2
echo "in a GIF viewer that can display animation in animated GIFs." 1>&2
echo 1>&2
echo "If you use macOS you can open them all in Safari, one tab per image, like: " 1>&2
echo 1>&2
echo "	open -a Safari prog.gif golem.gif smily.gif invisible.gif" 1>&2
echo 1>&2
echo "Once they're open make sure to watch as they change form." 1>&2
