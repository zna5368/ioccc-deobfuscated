# 2018/burton2

Difficulty: Terrifying

1. Data hidden in a string made only of braces and semicolons ("Freecode")

The long string after `#include W` holds only `{`, `}`, `;`, spaces and tabs.
The function `b()` reads it as base 3 numbers (`{` = 0, `}` = 1, `;` = 2) and skips
the spaces and tabs. From these numbers `main` builds the character class table,
the option flag table, the `getopt` option string `"?cikrst"` and the trigraph
table. No letters are written anywhere in the code. The trick also makes the
string free in `iocccsize`, because `{`, `;` and `}` followed by whitespace are
not counted. In simple.c these tables are written out as plain arrays and a
`switch` statement.

2. One memory block shared by many tables

The array `char M[2053]` holds everything. Bytes 0-255 are the character class
table, an `int` table of keyword group offsets (`q`) starts at byte 256, the
keyword letters start at byte 384, option flags are at byte 1024, the option
string at byte 1056 and the trigraph table at byte 1088. The pointers `s` and `O`
are moved by adding `32` to switch between the tables, so it is hard to see which
table is being used.

3. Character classes stored as bit flags

`M[c]&16` means whitespace, `&8` identifier letter, `&4` decimal digit, `&2` hex
digit, `&1` one of `;{}`, and `&64`, `&128`, `&32` mean an operator can be followed
by `=`, can be doubled (`++`, `&&`), or can make `->`, `<<=` and `>>=`. With only
numbers in the code a reader has to decode the table to know what is tested.
simple.c names the bits (`SPACE`, `IDENT`, `DIGIT`, ...).

4. Token codes built with bit arithmetic

Operators get their codes by OR-ing bits onto the character: `c|128` is
"followed by =" (`+=` is 171), `c|o` with `o` = 256 is "doubled" (`++` is 299), and
`c|o|128` is `<<=` or `>>=`. Comments, keywords, identifiers, integers and floats
are found by adding small numbers to `o` (`o+2`, `++o`, `o+5`, `!--z^4` which is
always 4). simple.c uses named constants.

5. `for` loops used as `if` and `else`

Most `if` statements are `for` loops that run once or not at all, such as
`for(;a&8;){ ... return ...; }`, `for(n=a; c; c^=c)` and `for(...; !T++; ...)`.
The loop body either returns or changes the loop variable so the loop stops. In
`f()` the line `for(c=v&2; !c; ...)` is the `else` branch of the `for(n=a; c; c^=c)`
before it. The program contains no `if`, `else`, `while` or `switch` at all.
simple.c uses normal `if`/`else` and `while` statements.

6. Long chains of `?:`, `&&`, `||` and the comma operator

Whole statements are written as one expression, such as the input loop
`i[o]=c,c==l+3?v&1&&++E:c==l&&i[~-o]==92?--o,++L,v&1?E+=2,++A:A:++o` which
handles carriage returns, backslash-newline and normal characters. The return in
`j()` is a five-way `?:` chain choosing between quotes, `op=`, doubled operators,
`->` and single characters. Understanding them needs a good knowledge of C
operator precedence. simple.c splits them into separate statements.

7. Trigraphs used in the code

The trigraph handling line is itself written with trigraphs:
`O??(c??)&&!(i??(??--o??)&i??(o-2??)??'63)` is `O[c]&&!(i[~-o]&i[o-2]^63)`. With
`-trigraphs` `??(` becomes `[`, `??)` becomes `]`, `??-` becomes `~` and `??'`
becomes `^`. The test `(i[o-1]&i[o-2])^63` is zero when both previous
characters are `?`, a shorter way to write the check.

8. Confusing names: O, 0, l, 1 and shadowed variables

Variables are named `O`, `l` and `I` so they look like the digits `0` and `1`.
`l` is the global newline value 10 but is also a parameter name in `h`, `d`, `g`
and `b`. `O`, `F`, `T`, `n` and `o` are global, yet functions declare parameters
or locals with the same names, so the same letter means different things in
different places (for example `F` is the keyword string, then the encoded
tables, then a pointer to `"\n"` in `main`, and a length in `f`). The global
variable names spell `sizeOF`, `voLATILE`, `cHaCKRZ` and so on.

9. Globals used to pass values between functions

`j()` leaves the token start in `s`, the whitespace start in `O` and the scan
position in `z`. `g()` leaves the whitespace flag of the last character in `a`,
and `f()` then uses it as the starting state for the keyword scan inside `//`
comments. Moving one statement changes the results, as the author warns. In
simple.c the needed value is computed where it is used.

10. Pointer and integer tricks

`-~l` is `l+1`, `~-o` is `o-1`, `z[~0]` is `z[-1]`, `~0` passed as a length means
"no limit", `~-Z>>2` is 63 (the `?` returned by `getopt`), and `~(a=getopt(...))`
is zero only when `getopt` returns -1. The return value of `fprintf` is turned
into 0 with `!` so the seven print calls can be added together in the final
`return`.

11. Reused buffers and pointer offsets

The input buffer starts at offset 2 (`o=2`, `i+=2`) so the trigraph check can
look two bytes back. The newline used to end each token line is written into
`M` right after the keyword letters (`*++e=l=10`) and printed through `F`. The
final `h(O,a)` in `main` always prints nothing because `f()` sets `a` to 0 first.

12. Keyword list kept in an external file

The reserved words come from `#include W` where `W` is set on the compiler
command line to a file that holds one string with the words grouped by length.
`main` copies the words into `M` and records where each length starts, and
`d()` only compares a word against keywords of the same length.
