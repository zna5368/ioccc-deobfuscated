# 1984/laman

Difficulty: Easy

1. Source laid out in tab-separated columns

The whole program is chopped into short pieces separated by tabs, so it looks
like a table instead of code. Tokens are split across "cells" and lines, for
example `b+=` and `e;` or `(k-(b+1))` broken over two lines. Running the code
through a formatter (as the judges suggest) restores normal statements.

2. Implicit `int` globals with one-letter names

`a[900]; b;c;d=1;e=1;f;g;h;O;` declares ten global variables with no type,
relying on old C defaulting to `int`. The names say nothing about their use:
`b` and `c` are the current column and row, `d` is the next number to place,
`e` is the direction (+1 or -1), `O` (a capital letter O that looks like zero)
is the length of the current run, `g` is the input limit, and `k` and `h` are
the number of columns and rows.

3. Reusing `main`'s parameters as ordinary variables

`main(k, l) char **l;` uses an old-style declaration. `l` is really `argv`, but
`k` (which is really `argc`) is immediately overwritten and used as the column
count, so the argument count is never actually looked at.

4. Work hidden inside empty `for` loops

`for(k=0;k*k<g;b=k++>>1);` has an empty body. It finds the smallest `k` with
`k*k >= g` (the column count, a rounded-up square root) and, in the increment
part, also sets the starting column `b` to `(k-1)/2`. The similar loop over `h`
computes a rounded-down square root.

5. Comparison result used as a number

`h += g > h*(h+1)` adds the result of a comparison (0 or 1) to `h`, which
means "add one more row if the numbers do not fit". It is then folded into
`c = ((h += ...) - 1) >> 1`, so two calculations happen in one line, and `>> 1`
is used instead of `/ 2`.

6. A 2D grid stored in a flat array using bit operations

`a[b<<5|c]` treats the 900-int array as a grid with 32 cells per column.
`b<<5` is `b*32` and `| c` adds the row because `c` is always less than 32.
This hides the fact that the program is working with rows and columns.

7. Comma operator to pack statements

`a[b<<5|c]=d++,b+=e` stores a number and moves one step in a single
expression, which lets each `for` loop have a body without braces.

8. XOR swap in a single expression

`a[x] ^= a[y] ^= a[x] ^= a[y]` swaps two cells without a temporary variable.
It is used to mirror each row left to right just before it is printed. Writing
all three XORs in one expression also modifies the same object more than once
without a sequence point, which is undefined behaviour that only happens to
work as a swap.

9. Conditional format string

`printf(a[...] ? "%-4d" : "    ", a[...])` chooses the format string with `?:`.
Empty cells (value 0) print four spaces and the number argument is ignored.
