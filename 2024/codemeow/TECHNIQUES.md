# 2024/codemeow

Difficulty: Hard

1. Data hidden inside `double` constants

The array `double O[]` holds 19 numbers such as `2.5673396845218159e-289`.
They are never used as numbers. Their raw bytes are read through a
`char *` cast (`((char*)&O)[E++]`, `(char*)(O+14)`), so the "numbers" are
really packed byte tables: the pot colour of each cell, the pot's ASCII art,
the ANSI colour codes `"32"`, `"92"`, ..., the segment strings `" /\"`,
`"\~\"`, `"&#&"`, two tables of small random steps, and the divisors
`1, 10, 100`. To read them you have to dump the bytes of the doubles in memory.

2. Byte offsets instead of named tables

Every table is found by doing pointer arithmetic on `O`: `O+14` is the colour
codes (2 bytes each), `O+16` is the segment strings (3 bytes each), `O+18` is
the digit divisors, `&O[12]` and `&O[13]` are the random step tables, and
`((char*)&O)+(_<<2)+24` is the pot art. Nothing has a name, so you have to
count bytes to know what each offset means.

3. Macro renaming a library function

`#define A write` makes `A(1,_,__)` a call to `write()`. In `t()` the same
letter is also used as a local variable (`int A,B,C,D`), which the macro turns
into a local variable called `write` that hides the real function.

4. Meaningless identifiers

Functions are single letters (`e`, `g`, `l`, `t`, ...), and parameters are
named `_` and `__`. Globals `a`, `b`, `c`, `d` are the screen width, screen
height, pot column and pot row, and `E` is a counter that is used for two
unrelated things: first it walks through the pot colours, then its odd/even
value picks the side of each new branch.

5. Raw byte buffers instead of structures

`gettimeofday()` writes into `char _[16]`, and only the first byte of the
seconds is used as the random seed. `nanosleep()` gets `char _[16]` with
`_[11]=1`, which on a 64-bit little-endian machine sets `tv_nsec` to
`1<<24` (about 16.8 ms). The window size from `ioctl(TIOCGWINSZ)` is read into
an `unsigned long` and split with `&65535` and `>>16` instead of using
`struct winsize`.

6. Hand-made number printing

`Q()` turns a number into one digit by dividing by a byte from the table
(`1`, `10` or `100`), then `% 10 + 48` (48 is `'0'`). `l()` uses it with
compound literals like `(char[]){Q(_,2),Q(_,1),Q(_,0)}` to print the
ANSI cursor move `ESC[row;colH` without `printf`.

7. Tiny hidden random number generator

`r()` does `v^=8; return ++v%8;` to make numbers from 0 to 7. Because `v`
comes from a signed `char`, it can start negative; then `++v%8` is negative
and turning it into an `unsigned char` gives 249..255. Those values read far
past the end of the step tables in `L()`, landing in zeroed memory, so the
first steps of the tree are 0 until `v` becomes positive.

8. Recursion as the growing algorithm

`t()` draws one segment per loop and calls itself to start new shoots
(`t(_,__,(E++%2)+1,y+5)`) and leaves (`t(_,__,4,y)`). The type of the
segment is a plain number 0..4, and `p()` uses a `switch` on it with
`default:` hiding the two shoot cases and a sign trick `(1-(_-1)*2)` that
turns type 1 into +1 and type 2 into -1.

9. Code shaped like a tree in a pot

The source is laid out in the shape of a bonsai in its pot, so lines are split
in odd places, such as in the middle of `E++ %2` or the `if(__ <0)` test,
which makes it hard to read statements as a whole.
