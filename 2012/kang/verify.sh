#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2012/kang

# make sure CC is set so that when we do make CC="$CC" it isn't empty. Doing it
# this way allows us to have the user specify a different compiler in an easy
# way.
if [[ -z "$CC" ]]; then
    CC="cc"
fi

make CC="$CC" all >/dev/null || exit 1

# clear screen after compilation so that only the entry is shown
clear

read -r -n 1 -p "Press any key to run: echo Nineteen hundred and eighty-four | ./simple # 1984: "
echo 1>&2
echo Nineteen hundred and eighty-four | ./simple
echo 1>&2

read -r -n 1 -p "Press any key to run: echo uno | ./simple # 1: "
echo 1>&2
echo uno | ./simple
echo 1>&2

read -r -n 1 -p "Press any key to run: echo trois | ./simple # 3: "
echo 1>&2
echo trois | ./simple
echo 1>&2

read -r -n 1 -p "Press any key to run: echo fier | ./simple # 4, should be vier: "
echo 1>&2
echo fier | ./simple # notice the issue here; this is intentional
echo 1>&2

read -r -n 1 -p "Press any key to run: echo vier | ./simple # 4: "
echo 1>&2
echo vier | ./simple # notice the issue he; this is intentional
echo 1>&2

read -r -n 1 -p "Press any key to run: echo \"shest'\" | ./simple # 6: "
echo 1>&2
echo "shest'" | ./simple
echo 1>&2

read -r -n 1 -p "Press any key to run: echo dos | ./simple # 2: "
echo 1>&2
echo dos | ./simple
echo 1>&2

read -r -n 1 -p "Press any key to run: echo forty-two | ./simple # 42: "
echo 1>&2
echo forty-two | ./simple
echo 1>&2
