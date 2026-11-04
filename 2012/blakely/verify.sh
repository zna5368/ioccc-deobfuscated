#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2012/blakely
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

read -r -n 1 -p "Press any key to run: ./simple 'xx*yy*+' 64 > paraboloid.gif: "
echo 1>&2
echo "This might take a bit of time, please be patient." 1>&2
./simple 'xx*yy*+' 64 > paraboloid.gif
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 9 32 > empty.gif: "
echo 1>&2
./simple 9 32 > empty.gif
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 'xy*' 32 > saddle.gif: "
echo 1>&2
echo "This might take a little bit of time, please be patient." 1>&2
./simple 'xy*' 32 > saddle.gif
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple 'xx*yy*1++d5*ct/' 64 > ripple.gif: "
echo 1>&2
echo "This might take a bit of time, please be patient." 1>&2
./simple 'xx*yy*1++d5*ct/' 64 > ripple.gif
echo 1>&2

read -r -n 1 -p "Press any key to run: time ./simple 0 250 > flat.gif: "
echo 1>&2
echo "This might take a while, please be patient." 1>&2
time ./simple 0 250 > flat.gif
echo 1>&2

echo "Now open paraboloid.gif, empty.gif, saddle.gif, ripple.gif and flat.gif in a" 1>&2
echo "graphics viewer that ANIMATES ANIMATED GIFs." 1>&2
