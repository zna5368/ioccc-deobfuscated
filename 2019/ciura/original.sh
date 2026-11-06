#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2019/ciura
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

echo "$ ./getwords.sh en | grep .. | ./prog aeiouvwxyz"
read -r -n 1 -p "Press any key to continue (space = next page, q = quit): "
echo 1>&2
./getwords.sh en | grep .. | ./prog aeiouvwxyz 
echo 1>&2

echo "$ ./getwords.sh en | grep .. | ./prog aeiouhjklmnvwxyz" 1>&2
read -r -n 1 -p "Press any key to continue (space = next page, q = quit): "
echo 1>&2
./getwords.sh en | grep .. | ./prog aeiouhjklmnvwxyz 

echo "$ ./getwords.sh en | grep .. | ./prog mrjocktvquizphdbagsfewlynx | head -n 256" 1>&2
read -r -n 1 -p "Press any key to continue (space = next page, q = quit): "
echo 1>&2
./getwords.sh en | grep .. | ./prog mrjocktvquizphdbagsfewlynx | head -n 256 


echo "$ ./en.sh" 1>&2
echo "There will likely be a lot of output. Quit when bored. No logophiles will be hurt." 1>&2
read -r -n 1 -p "Press any key to continue (space = next page, q = quit): "
./getwords.sh en | grep .. | ./prog abcdefghijklmnopqrstuvwxyz | head -n 100000
./getwords.sh en | grep .. | ./prog abcdefghijklmnopqrstuvwxyz 7 | head -n 2000
printf 'ab ba\n\nabc  c ab\r\nca cab b a x\nbc' | ./prog abc
printf 'ab ba\n\nabc  c ab\r\nca cab b a x\nbc ' | ./prog cab 2
echo | ./prog ''
./prog 2>&1; echo "exit $?"
