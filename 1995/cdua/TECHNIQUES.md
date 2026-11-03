# 1995/cdua

Difficulty: Medium

1. Three functions packed into one `main()`

`main()` builds the board, carves the maze and solves it. It calls itself
through the pointer `n=main` for every recursive step. The global `c` records
which job is running: `0` means the board has not been built, `1839` means it
was just built and must be printed, `1840` means carving, and `1` means
solving. Every call checks `c` to find out which part of the body to run.
`simple.c` splits this into `init_board()`, `carve()` and `solve()`.

2. Parameters reused as variables

`main(int a,char**j)` uses `a` as the current board position, not as the
argument count. The top call overwrites it with `1000`, the start cell, and
recursive calls pass a position as `a` and `0` as `j`, which is never read.

3. Function pointers hiding library calls

`k=rand, l=time, m=getchar, n=main` store the functions in variables with
one-letter names. `k()%4`, `l(0)`, `m()` and `n(a+1,0)` do not look like calls
to `rand`, `time`, `getchar` and `main`.

4. A one-dimensional array used as a 2D grid

The board `b[1841]` is 23 rows of 80 characters in one string. Moving right
or left is `+1` or `-1` and moving down or up is `+80` or `-80`. A position is
turned into a screen row and column with `a/80+1` and `1+a%80`. Magic numbers
such as `1000` (row 12, column 40), `162` (row 2, column 2), `1676` (row 20,
column 76) and `1840` (the board size) are all positions or sizes on this grid.

5. A boolean used as a string index

`" #\n"[d%80==79?2:d/80&&d%80&&d/80-22&&d%80-78]` picks the character for
each cell. The last column gets index 2, a newline. Otherwise the `&&` chain
is 1 for inside cells (`'#'`) and 0 for the border (`' '`). The loop that does
this has an empty body. All the work is in the `for` header.

6. Subtraction instead of comparison

`!(c-1839)`, `!(g-a)` and `if(c-1){}else` all test for equality: `x-y` is
zero only when `x==y`. The empty `{}` with `else` turns "if not equal" around
into "if equal".

7. Adding characters to test the neighbours

`b[a+2]+b[a-2]+b[a+160]+b[a-160]-4*' '` adds the four cells two steps away.
Each of them is either a space or `'#'`. If all four are spaces, the sum is
exactly `4*' '` and the result is zero. Any `'#'` makes it non-zero, so the
expression means "some neighbour is still solid wall".

8. Nested ternary as a lookup table

`(e=k()%4)?e-1?e-2?-1:1:-80:80` turns a random number 0 to 3 into a step of
`80`, `-80`, `1` or `-1`. It sits inside the array index of a `while`
condition with an empty body, so choosing a direction and testing it happen in
one line.

9. `goto` used as a loop

The label `k:` and the `goto k` at the end of the carving block form a
`while` loop. The label also has the same name as the function pointer `k`.
That is allowed because labels have their own namespace.

10. The whole maze solver in one expression

The `return` statement solves the maze using `?:`, the comma operator and
`||`. The comma operator runs its parts in order, so it can mark a cell,
print messages and move the cursor. `n(a+1,0)||n(a+80,0)||n(a-80,0)||n(a-1,0)`
tries right, down, up and left. Because `||` stops at the first true value,
the search ends as soon as one direction reaches the exit. If none does, the
last part of the ternary erases the dot and returns 0 to back up.

11. Several cells drawn by one `printf`

The format `v` holds three cursor-move-and-print sequences, so a single
`printf(v,...)` with nine arguments clears the wall, the gap and the next
room. `simple.c` calls `draw_cell()` three times instead.

12. Strings split across lines and named with single letters

The messages `u0` to `u5` and the escape codes `o`, `x`, `y`, `z`, `w`, `v`
are global pointers with no meaning in their names. Some of them are broken
across source lines with a backslash, for example `"Going back..\` and `."`.

13. Hidden side effect of the shared start check

`else if(!(g-a))` is meant to run once, when the top call finishes carving at
cell `1000`. It is still checked during solving. If the solver walks onto
cell `1000`, the program asks for `<RET>` again and jumps back to the entrance
`162`. The entrance already has a dot, so that cell acts like a wall.
`simple.c` keeps this behaviour in `solve()`.
