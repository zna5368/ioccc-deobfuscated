# 1995/dodsond2

Difficulty: Medium

1. Code shaped as ASCII art

The source is laid out in a picture with large blank holes, and tokens are
split across lines in odd places, for example `J` at the end of one line and
`(2)` at the start of the next, or `mvaddch(` hanging off a line. The compiler
ignores the layout, but a reader cannot see where statements begin and end.
Empty comments such as `clear(/*|*/)` and `refresh(/*v*/)` are only there to
fill out the shape.

2. Bit shift macro instead of plain numbers

`#define J(x) (1<<x)` is used for almost every small number. `J(0)` is 1,
`J(1)` is 2 and `J(J(3))` is `1<<8`, which is 256, the "visited" flag. Even
the step east is written `x+=J(3)-J(0)`, which is 8 - 1 = 7. simple.c uses
the plain numbers and names the flags (`OPEN_NORTH`, `VISITED`, ...).

3. Hexadecimal for common decimal numbers

The size of one level, 49, is written `0x31`, and the cube size 343 is
written both as `343` and as `0x157`. Mixing the two forms hides that these
are the same few numbers (7, 49 and 343). simple.c names them `SIZE`,
`LEVEL_CELLS` and `CELLS`.

4. A 3D maze flattened into one array

The 7x7x7 maze is the array `q[343]`, and a cell's row, column and level are
read with `x%7`, `(x/7)%7` and `x/49`. Moving is done by adding a number from
`u[6]={-1,7,49,-49,-7,1}`. The directions are ordered so that the opposite of
direction `d` is always `5-d`, which lets the carving code open the wall on
both sides with `q[x]+=J(d)` and `q[x]+=J(5-d)`. simple.c names the
directions and adds `ROW`, `COLUMN` and `LEVEL` macros.

5. Counting true comparisons to check a move

To make sure a step does not wrap around the edge of a row or level, the
program adds three comparisons together, `(same row)+(same level)+(same
column)==J(1)`. In C a comparison is 1 when true and 0 when false, so the sum
is 2 only when exactly one coordinate changed. simple.c keeps the trick but
compares against a plain `2` and stores the neighbour in `next`.

6. Weighted random choice hidden in a table

`r[27]` uses the macro `U` (`0,1,4,5`) five times, so the table looks like
padding. It is a lookup table for `r[rand()%27]`: the four flat directions
appear six or seven times each but up and down (2 and 3) appear only once, so
the maze mostly grows sideways and has few stairs. simple.c writes the table
out with direction names.

7. Drawing macro with parameters that do nothing

`P(A,B,C,D,E)` draws one character of a cell as `(q[y]&C)?D:E`. The four
corners are drawn with `C` set to 0, so `q[y]&0` is always false and the
corner is always `#`. These calls look like real wall checks but are just a
long way of writing `'#'`. The wall character itself is stored in the
variable `j` and a space is hidden behind `#define V ' '`.

8. Switch cases built by a macro

`#define O(A,B,C) case A:if(q[x]&B)C;break;` writes a full `case` label, a
wall check and a `break`, so the `switch` holding the keyboard handling does
not visibly contain any `case` or `break`. The up and down cases pass two
statements joined with a comma, `(x+=49,clear())`, as a single argument.
simple.c writes each case out in full.

9. Short single-letter globals reused for many jobs

All variables are globals named `q`, `x`, `y`, `d`, `l`, `a`, `b` and `j`.
`x` is the loop counter when clearing the maze, then the carving position,
then the player's position, and finally the "did you escape" flag, because
the game ends when `x` reaches 0 (escaped) or is set to -1 by `q` (quit).
simple.c renames them (`maze`, `pos`, `cell`, `remaining`, `left`, `top`).

10. Random jumps hidden behind a macro

`else if(R<R/0x7)` compares two random numbers, one divided by 7, so the
walk only sometimes jumps to a random carved cell when the chosen neighbour
is already part of the maze.
`R` is a macro for `rand()`, which makes it look like a variable. simple.c
writes `rand() < rand() / 7` directly.
