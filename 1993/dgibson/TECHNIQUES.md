# 1993/dgibson

Difficulty: Catastrophic

1. Data file that is also source code

The board in `life.d` (`LIFE`, `L _ _ O _`, `GEN 0`, `END`) is pulled into the
program with `#include "life.d"`. Every word in the board is a macro, so the
board itself becomes the body of `main()`. The program prints the next board,
`dgibson.sh` saves it over `life.d`, and the program must be compiled again for
every generation.

2. One macro per cell

Each `_` and `O` expands into `X(0)` or `X(1)`. `X(p)` is the code that works
out the next state of that one cell and prints it. A 5 by 5 board turns into 25
copies of the life rule, so the compiled program changes size with the board.

3. Whole program in a single expression

`main()` is one `return` statement made of `||`, `&&`, `?:` and the comma
operator. `a || b` runs `b` only when `a` is false, and `a && b` runs `b` only
when `a` is true, so they are used in place of `if` statements. Assignments and
`printf()` calls sit inside the expression and are given a value of `0` so the
chain keeps going.

4. Unbalanced parentheses split across macros

`L` starts with `endb)` to close the group opened by the row before it, and ends
with `(lnst` to open a group for its own row. `LIFE` opens the first group and
`GEN` closes the last one and the function. No macro makes sense on its own,
only after the whole board has been expanded.

5. `?:` that swallows the rest of the row

`lnst` is `doln ? 0 : ...`. Because `?:` has very low precedence, the part after
`:` reaches all the way to the end of the row, so one test skips or runs every
cell of the row. This is how rows are dropped and how a search ignores rows it
is not looking for.

6. `main()` recursing in two directions

`main(col, row)` with `col > 1` switches into a search mode (`find`) that walks
the same board expression to add up three cells of row `row` around column
`col`. Each cell calls this three times to count its neighbours. `main(-col,
row)` with a negative column builds a blank row one cell at a time by calling
itself until it has enough cells. `main(1, argv)` is the normal start.

7. Search ends by short-circuit

In search mode `X(p)` returns a true value as soon as it reaches the wanted
column. That true value stops every `||` that follows it, so the rest of the
board is never looked at and the sum in `ret` is returned.

8. Global variables as shared registers

`bat`, `bit` and `but` hold the last three cells seen during a search, `las`
holds the last cells printed, and `rcl`, `rcf`, `rro`, `rrc`, `rrr` carry
values between recursive calls. Some of these are read by nested calls they
were not meant for. This makes the blank row added at the bottom lose its new
left cell and report one cell too many when the right column is dropped.
`simple.c` copies both quirks so its output is the same.

9. Bit-packed status number

`STAT` is the width, height and eight edge flags in one number
(`width << 16 | height << 8 | flags`), then XORed with `0x66` so that "the two
edge rows are empty" reads as set bits. The next run uses it to add a blank row
or column where the pattern touches an edge and drop one where two are empty.
The flags are tested with masks such as `(prs & 12) == 4`.

10. Short meaningless names

`bat`, `bit`, `but`, `las`, `ccl`, `cro`, `dfc`, `dfl`, `prs` and macros like
`endb`, `lnst`, `chpa`, `eols`, `recu` and `addl` do not say what they hold or
do. `main()` also uses `argc` as a column number and `argv` as a row number.

11. Name clash between the data and globals

`GEN 0 END` turns into `int gen = 0 ;` and `STAT n` into `long prs = n`. Both
variables were already declared at the top, so the data quietly gives them
their starting values. With no `STAT`, `prs` stays `0` and nothing is resized.
