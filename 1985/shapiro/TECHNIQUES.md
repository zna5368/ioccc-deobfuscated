# 1985/shapiro

Difficulty: Hard

1. Single letter global names

Every variable is a single letter global (`M`, `N`, `a`, `l`, `m`, `b`, `d`,
`c`, `e`, `f`, `g`, `i`, `j`, `k`, `s`). `M` is the maze, `N` is a queue of
cells, `m` holds the step to move east/north/west/south and `l` holds the
column a neighbour must not be in (to stop a move wrapping to the other side
of the grid). Nothing in the names says what any of them hold.

2. Variable reuse

The same variable is used for unrelated jobs. `f` is first the number of
possible moves, then the value of the chosen neighbour cell, then the new
value written into it. `j` remembers whether the two halves of the maze have
been joined, and later stores the return value of `write()` from the `P()`
macro. `s` is a direction during generation and a column counter while
printing the top border, and `e` is both a cell index and a column counter.

3. Bit packed cells

Each maze cell is a number whose bits mean "open to the east" (1), "open to
the north" (2), "open to the west" (4) and "open to the south" (8). Bit 16
marks cells that belong to the tree grown from the exit. Magic constants like
`{2}` (the entrance) and `24` (exit open to the south plus the 16 mark) hide
this, and `1<<s` and `1<<(s+2)%4` open the wall in the chosen direction and
the opposite direction.

4. Two interleaved walkers

Instead of one random walk, two walkers take turns: one starts at the exit
(bottom right) and one at the entrance (top left, cell 0, because `e` starts
at zero). This is hidden in `c=M[g=i];i=e;` which processes cell `i` and then
shuffles the "next" cell `e` into `i`, so the two positions alternate. The two
trees are joined the first time a walker touches the other walker's tree,
which is detected with `!j&&c>=16!=M[k]>=16`.

5. Pointer queue disguised as a stack

`*d++=e` pushes every carved cell and `e=d>b++?b[-1]:e` pops the oldest
one when a walker is stuck. Because `b` is always incremented (even when the
queue is empty) the loop condition `f|d>=b` ends the program once the queue
runs out.

6. Arithmetic in place of conditionals

`f+=c&-16*!j` copies the region mark from the current cell only before the
trees are joined: `-16*!j` is either `-16` (all bits above 15 set) or `0`.
`j=j<f?f:j` keeps the largest cell value seen so that `j` becomes non-zero
the moment a walker enters a cell that was already used.

7. Random number scaling

`rand()/(1+2147483647/f)` picks a random index from `0` to `f-1` by dividing
the full range of `rand()` into `f` equal buckets. The number `2147483647` is
`RAND_MAX` written out, so no header is needed.

8. String literal indexing

`"| "+(*u/4)%2` and `"_ "+(*u++/8)%2` add 0 or 1 to a string pointer, so
`write()` prints either the wall character or a space depending on whether
the west or south bit is set in the cell.

9. Output macro with side effects

`#define P(X)j=write(1,X,1)` hides each one character `write()` call and
quietly assigns its result to `j`. The macro is also placed in the increment
part of `for` loops, such as `for(s=C;--s;P("_"))P(" ");` and
`for(;P("\n"),R--;P("|"))`, so the order of the printed characters is hard
to follow.

10. Packed loops and missing headers

The whole program is a few long lines with `for` loops that have no braces
and conditions joined by `&&`, `||` and `|`. The bounds checks
`k>=0&&k<C*R&&l[s]!=k%C` are written inline in the condition. No headers are
included, so `rand()` and `write()` are implicitly declared and `main()` uses
implicit `int`.
