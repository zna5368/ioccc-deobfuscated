#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 1994/westley
#

# make sure CC is set so that one can control what compiler to use in an easy
# way.
if [[ -z "$CC" ]]; then
    CC="cc"
fi

# check for 80 columns
COLS="$(tput cols)"
if [[ "$COLS" != 80 ]]; then
    echo "WARNING: without a terminal with 80 columns, 8 character tabs and wraparound" 1>&2
    echo "this will not look that great. We have detected that you do not have your terminal" 1>&2
    echo -n "at 80 columns. Do you still wish to continue (Y/N)? "
    read -r -p ""
    if [[ "$REPLY" != "y" && "$REPLY" != "Y" ]]; then
	exit 1
    fi
fi

clear

"${CC}" -Describe -Door simple.c
"${CC}" -Describe -Drain simple.c
"${CC}" -Describe -Dwarf simple.c
"${CC}" -Describe -Debris simple.c # or -Dungeon
"${CC}" -Describe -Desk simple.c
"${CC}" -Describe -Dime simple.c
"${CC}" -Describe -Directory simple.c
"${CC}" -Dial -DUNgeon0614 simple.c
"${CC}" -Drink -Daiquiri simple.c
"${CC}" -Drop -Daiquiri simple.c
"${CC}" -Drop -Dwarf -Daiquiri simple.c # or -Drop -Down -Daiquiri
"${CC}" -Describe -Document simple.c
"${CC}" -Depress -Dotted -Dog simple.c
"${CC}" -Depress -Dalmatian simple.c # also accepts "Dalmation" [sic]
"${CC}" -Deposit -Dime simple.c
"${CC}" -Drop -Donut simple.c
"${CC}" -Drop -Donut -Down -Drain simple.c # or -Drop -Dwarf -Donut
