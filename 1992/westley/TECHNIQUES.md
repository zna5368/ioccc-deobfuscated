# 1992/westley

Difficulty: Easy

1. Run-length encoded map hidden in a string

The map is not stored as a picture. Each character in the string is a count:
`'A'` means 1, `'B'` means 2 and so on, found by counting down until the
character reaches `'@'` (64) in `for(;n-->64;)`. The counts take turns between
land (`'!'`) and sea (`' '`), so `"@NKA..."` means 0 land, 13 sea, 10 land, 1
sea, and so on. Any character at or below `'@'` is a count of zero, which only
switches to the next kind of run.

2. Decoy text inside the data

The end of the map string is `" H E L L O,    W O R L D! "`. It looks like a
greeting but it is map data. The letters are counts of sea (`'H'` is 8, `'O'`
is 15) and the spaces, comma and `!` are below `'@'`, so they are empty land
runs. Together they print the last rows of sea.

3. Picking the character with arithmetic

`putchar(!d+++33^l&1)` prints one of four characters with no `if`. `!d` is 1
only at the marked spot, so `33 + !d` is `'!'` (33) or `'"'` (34). `l&1` is 1
on sea runs, and XOR with 1 changes `'!'` to `' '` and `'"'` to `'#'`. The
reader also has to know that `+` and `&` are done before `^`, and that `d+++33`
means `d++ + 33`.

4. Counting down to the marker

The latitude and longitude are turned into a position in the output, but the
code stores the negative of it in `d` and adds 1 after every character. The
marker is printed when `d` reaches zero, so there is no comparison with a
named position anywhere.

5. Reusing `argc` as the loop index

The first parameter of `main` (normally `argc`) is called `l` and is reused to
step through the map string with `[l++-3]`. With two arguments `argc` is 3, so
the index starts at 0. After `l++`, `l` is the index plus 4, which has the same
odd or even value as the index, and that is what `l&1` uses to choose land or
sea. Running with extra arguments starts in the wrong place in the map.

6. Indexing a string literal directly

`n="@NKA..."[l++-3]` reads a character straight out of a string constant
without giving it a name, and the assignment is used as the `for` loop test so
the loop stops when it reads the `'\0'` at the end.

7. Old style declarations and implicit `int`

`main(l,a)char**a;` uses the pre-ANSI way of declaring parameters, `l` has no
type so it defaults to `int`, and `atoi` and `putchar` are used without any
`#include`. `main` also never returns a value.

8. Shaped source layout

The code is laid out in the shape of a globe-like blob, and the map string is
split over several lines with backslash-newline joins, so the string and the
loops are hard to pick out.
