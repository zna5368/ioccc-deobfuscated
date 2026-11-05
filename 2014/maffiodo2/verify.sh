#!/usr/bin/env bash
#
# try.sh - demonstrate IOCCC entry 2014/maffiodo2
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

echo "$ ./simple '  .,:;!$#@' < image.rgb" 1>&2
read -r -n 1 -p "Press any key to continue (space = next page, q = quit): "
echo 1>&2
./simple '  .,:;!$#@' < image.rgb 
echo 1>&2

echo "$ ./simple \"  .:;Y0\" < image.rgb" 1>&2
read -r -n 1 -p "Press any key to continue (space = next page, q = quit): "
echo 1>&2
./simple "  .:;Y0" < image.rgb 

echo "$ ./simple ' :1' < image.rgb" 1>&2
read -r -n 1 -p "Press any key to continue (space = next page, q = quit): "
echo 1>&2
./simple ' :1' < image.rgb 

echo "$ ./simple '  .:;Y0' < image.rgb" 1>&2
read -r -n 1 -p "Press any key to continue (space = next page, q = quit): "
echo 1>&2
./simple '  .:;Y0' < image.rgb 

echo "$ ./simple ' .:-=+*#%@' < image.rgb" 1>&2
read -r -n 1 -p "Press any key to continue (space = next page, q = quit): "
echo 1>&2
./simple ' .:-=+*#%@' < image.rgb 

echo "$ ./simple '   .,:!-iots&8%#@$' < image.rgb" 1>&2
read -r -n 1 -p "Press any key to continue (space = next page, q = quit): "
echo 1>&2
./simple '   .,:!-iots&8%#@$' < image.rgb 
