# 1993/rince

Difficulty: Hard

1. Source file used as its own data file

With no argument the program opens `__FILE__` (its own source code), so the
Space Invaders game is stored in the big comment at the top of `rince.c`.
The first lines are the rules and the end of the comment is the map. The
C code after the map is never read, because `fread()` stops after
`width * height` bytes. In this file the wildcard character is a TAB
(`d=open-2?'\t':'*'`), so all the tabs in the comment are part of the game.

2. Misleading parameter names

`main(int open, U**exit)` names `argc` `open` and `argv` `exit`, which are
the names of real library functions. `open-2?__FILE__:*++exit` reads like a
function call but is just "if argc is not 2, use this file, otherwise use
argv[1]".

3. Macros that hide keywords

`U` is `char`, `G` is `for(` and `I` is `)malloc(sizeof(`. Because `G` opens a
`for` without its closing parenthesis, the matching `)` is many lines later,
for example `G clear(),noecho(),cbreak(); ;\n)`. The `I` macro turns
`X=(U**I P++)*q*(i+1))` into `X=(char**)malloc(sizeof(P++)*q*(i+1))`.

4. sizeof on expressions that are never evaluated

`sizeof` only looks at the type of its operand and does not run it. So
`sizeof(P++)` does not change `P`, and `sizeof(*a(Q--,--Q))` does not call
the function `a()` or change `Q`; it is just `sizeof(j)`, the size of a
rule.

5. setjmp/longjmp used as a loop

`G; setjmp(E)<256; Q-- ) longjmp(E,(J[Y][Q]=0,Q++));` looks like a loop that
counts down, but `Q--` never runs. Each `longjmp()` jumps back into the
`setjmp()` in the loop condition, which then returns the value passed to
`longjmp()`. The loop therefore clears `J[Y][0]` up to `J[Y][256]` (one past
the end) and stops when `setjmp()` returns 256. Since global arrays already
start as zero, this loop does nothing useful. A `goto L` jumps back to it
after the first blank line to "clear" the second rule list.

6. Variable shadowing inside declarations

`int x=0,y=0,W=Q,Q=i-1,k;` copies the parameter `Q` (the width) into `W`
and then declares a new `Q` (height - 1) that hides the parameter. Later
`int i=k&2,u=9,k=1,...` reads the outer `k` before declaring a new inner
`k`. `k&2` is always 0 because the outer `k` is only ever 0 or 1.

7. Two-dimensional loops packed into a single loop condition

The drawing loop `(mvaddch(y,x,X[y][x]),++x^W-1)||(x=0,y++)` and the cell
scanning loop
`k?++x-W||(x=1,++y-Q||(--k,x=W-1,y=Q-1)):--x||(y--,x=W-1),k|y` use `||` and
`?:` to update both the column and row counters, and even switch from the
forward scan to the backward scan, all in the condition part of a `for`.
`k` says which direction is being scanned and also picks the rule list with
`J[1-k]`.

8. An int used as an fd_set

`select(k,&y,0,0,...)` passes the address of the row counter `y` as the set
of file descriptors to watch. `k=y=- --x` sets `x` to -1 and both `k` and
`y` to 1, so `y` has bit 0 set (standard input) and `k` (1) is the number of
descriptors. After `select()` returns, `y` is 1 if a key is waiting and 0 if
not, and that value is then used as the first row of the scan.

9. Neighbour offsets encoded in a string

`P="noopoqqnr"` holds the steps between the eight neighbours of a cell.
`_+=P[T=i>u?u:i]-'o'` and `w+=P[4+T]-'p'` turn the letters into -1, 0, +1 or
±2 steps, and `T=i>u?u:i` folds the index so the same 5 letters can be used
from both ends. Walking these steps visits the neighbours in the order
top-left, top, top-right, left, right, bottom-left, bottom, bottom-right.
The same string ends in `"r"`, so `P+8` is reused as the `fopen()` mode.

10. One loop doing both matching and replacing

The neighbour loop runs twice. While its local `k` is 1 it checks that every
neighbour matches the rule and `break`s if one does not. When the counter `u`
reaches zero, the loop condition writes the new center cell, steps the
position back to the center with `w---_--` (which is `w-- - _--`), resets the
counters and decrements `k`, so the same loop body then writes the
replacement neighbours.

11. Comparisons written with subtraction and bitwise operators

`d-f[i]&&f[i]-X[_][w]` means `d != f[i] && f[i] != X[_][w]`, `++x-W` means
`++x != W`, `++x^W-1` means `++x != W-1` and `d==f[17]|f[17]==c` uses a
bitwise `|` instead of `||`.

12. Map rows sharing one memory block

`X[Q]=(U*)&X[q]+Q*i` places the text of the map directly after the array of
row pointers, so a single `malloc()` holds both, and a single `fread()` fills
every row, newlines included.
