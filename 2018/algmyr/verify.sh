#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2018/algmyr
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

read -r -n 1 -p "Press any key to run: ./simple -h | ./simple -d 1 0: "
echo 1>&2
./simple -h | ./simple -d 1 0
echo 1>&2

# Who needs cat? Answer: more people than you think.
read -r -n 1 -p "Press any key to run: ./simple prog.c | ./simple -d 1 0 (space = next page, q = quit): "
echo 1>&2
./simple prog.c | ./simple -d 1 0 
echo 1>&2

APLAY="$(type -P aplay)"
SOX="$(type -P sox)"

if [[ -z "$SOX" && -z "$APLAY" ]]; then
    echo "Missing both sox and aplay, cannot do this step." 1>&2
    echo "Tip: visit: https://www.ioccc.org/faq.html#sox" 1>&2
elif [[ -n "$SOX" ]]; then
    echo "$ echo -n ' MENE MENE TEKEL UPHARSIN ' | ./simple | \
$SOX -t raw -c 1 -r 44100 -L -e floating-point -b 32 \
- -n spectrogram -d 10 -X 300" 1>&2
    echo -n ' MENE MENE TEKEL UPHARSIN ' | ./simple |\
	$SOX -t raw -c 1 -r 44100 -L -e floating-point -b 32 - -n spectrogram -d 10 -X 300

    echo "Now open spectrogram.png in an image viewer or editor." 1>&2
    echo 1>&2
    read -r -n 1 -p "Press any key to continue: "
    echo 1>&2
    echo "$ echo \"IOCCC 2018\" | ./simple | $SOX -t raw -c 1 -r 44100 -L -e floating-point -b 32 - ioccc.wav"
    read -r -n 1 -p "Press any key to continue: "
    echo 1>&2
    echo "IOCCC 2018" | ./simple | "$SOX" -t raw -c 1 -r 44100 -L -e floating-point -b 32 - ioccc.wav
    echo "Now listen to ioccc.wav." 1>&2
else
    echo "$ echo \"<<<<<<  /\_/\_/\  _-_-_-_  !!!!!  :.:.:.:  >>>>>> ****** ~~~~~~~\" |
./simple | $APLAY -c1 -fFLOAT_LE -r44100" 1>&2
    read -r -n 1 -p "Press any key to continue: "
    echo 1>&2
    echo "<<<<<<  /\_/\_/\  _-_-_-_  !!!!!  :.:.:.:  >>>>>> ****** ~~~~~~~" | ./simple | "$APLAY" -c1 -fFLOAT_LE -r44100

    echo "Now open spectrogram.png in an image viewer or editor." 1>&2
fi
echo 1>&2

read -r -n 1 -p "Press any key to run: echo \"IOCCC 2018!\" | ./simple | ./simple -d 1 0: "
echo 1>&2
echo "IOCCC 2018!" | ./simple | ./simple -d 1 0

read -r -n 1 -p "Press any key to run: ./simple README.md prog.c > out.raw: "
echo 1>&2
./simple README.md prog.c > out.raw
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple -d 2 0 out.raw (space = next page, q = quit): "
echo 1>&2
./simple -d 2 0 out.raw 
echo 1>&2
