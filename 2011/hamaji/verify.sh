#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2011/hamaji
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

read -r -n 1 -p "Press any key to run: ./simple < dragon.nono (q = quit, space = next page): "
echo 1>&2
./simple < dragon.nono 
echo 1>&2

rm -f dragon dragon2
PERL="$(type -P perl)"
if [[ -n "$PERL" && -f "$PERL" && -x "$PERL" ]]; then
    echo "Perl installed, will generate output of ./simple < dragon.nono."
    echo 1>&2
    echo "$ $PERL c2nono.pl hamaji.c | ./simple | tee dragon"
    read -r -n 1 -p "Press any key to continue (space =  next page, q = quit): "
    "$PERL" c2nono.pl hamaji.c | ./simple | tee dragon 
    echo 1>&2
    echo "$ ./simple < dragon.nono | tee dragon2" 1>&2
    read -r -n 1 -p "Press any key to run: ./simple < dragon.nono | tee dragon2 (space = next page, q = quit): "
    echo 1>&2
    ./simple < dragon.nono | tee dragon2 
    read -r -n 1 -p "Press any key to run: diff -s dragon dragon2: "
    echo 1>&2
    diff -s dragon dragon2
else
    echo "Perl not found, will not run perl related steps." 1>&2
fi
rm -f dragon dragon2
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < random.nono (space = next page, q = quit): "
echo 1>&2
./simple < random.nono 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < soccer.nono (space = next page, q = quit): "
echo 1>&2
./simple < soccer.nono 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < codegolf.nono (space = next page, q = quit): "
echo 1>&2
./simple < codegolf.nono 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < face.nono (space = next page, q = quit): "
echo 1>&2
./simple < face.nono 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < penguin.nono (space = next page, q = quit): "
echo 1>&2
./simple < penguin.nono 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < samurai.nono (q = quit, space = next page): "
echo 1>&2
./simple < samurai.nono 
echo 1>&2

echo "$ ./simple < conway-game-of-life.nono" 1>&2
read -r -n 1 -p "Press any key to continue (space = next page, q = quit): "
echo 1>&2
./simple < conway-game-of-life.nono 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < no-solution.nono (no solution): "
echo 1>&2
./simple < no-solution.nono
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < multi-solutions.nono (> 1 solution): "
echo 1>&2
./simple < multi-solutions.nono
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < plus.nono (space = next page, q = quit): "
echo 1>&2
./simple < plus.nono 
echo 1>&2

read -r -n 1 -p "Press any key to run: ./simple < smiley.nono (space = next page, q = quit): "
echo 1>&2
./simple < smiley.nono 

exit 0
