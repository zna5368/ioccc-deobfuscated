# 2020/endoh1

Difficulty: Hard

1. Cell state packed into bits with magic numbers

Each cell is one `int` in `M`, and its state is spread over separate bits: 1 =
mine, 16 = still covered, 256 = flagged and 4096 = a temporary solver mark. The
code never names these bits. It uses sums of them as plain numbers: `273` is
256+16+1, `272` is 256+16, `257` is 256+1, `4368` is 4096+256+16 and `4112` is
4096+16. So `M[p]&=257` means "uncover the cell", `!(M[q]&272)` means "the cell
is probed" and `(4368&M[n])==4112` means "marked, covered and not flagged".
simple.c gives the bits names (`MINE`, `COVERED`, `FLAGGED`, `MARK`).

2. Several counters inside one sum

`F()` adds up the values of all cells in a 3x3 block. Each state bit sits in its
own group of 4 bits, and a block has at most 9 cells, so the groups do not spill
into each other. The one number returned holds four counts: `r%16` is the mines,
`r/16%16` the covered cells, `r/256%16` the flags and `r/4096` the marked cells.
`K()` takes these apart with expressions like `(g+f/256)%16` that mix two counts
in one step. simple.c uses the macros `MINES_IN`, `COVERED_IN`, `FLAGS_IN` and
`MARKED_IN`.

3. XOR toggling to find "neighbors of one cell but not the other"

`F()` also flips (XOR) the 4096 mark bit on each covered neighbor:
`M[q]^=(M[q]&16)<<8` shifts the 16 bit up to 4096. Flipping twice undoes the
change. After `F(p)` and then `F(q)`, cells next to both `p` and `q` have been
flipped twice and are unmarked again. Only cells next to exactly one of them
stay marked. That is why the solver calls `F(q)` and `F(p)` a second time: to
clear the marks again. With this trick one comparison in `K()` handles all four
rules from the README. Passing `0` instead of a second cell gives rules 1 and 2.

4. Macros that hide loops and calls

`O()` is a pair of nested `for` loops over the 3x3 neighborhood. The trick
`y-=!!y` steps back one row unless already at row 0 (`!!y` is 1 for any
non-zero `y`). `Y(n)` expands to `attrset(COLOR_PAIR(n)),mvprintw(` and leaves
the call open, so the arguments come after the macro. `_(x,y)` uses `##` token
pasting to build `COLOR_RED,COLOR_BLACK` from `_(RED,BLACK)`. simple.c writes the
loops and calls out in full.

5. Nested ternaries with comma operators

The character for each cell is chosen by one expression:
`k=E?256&M[p]?n--,2:E-2||M[p]%2<1?M[p]&16?q=p,m++,3:4+F(p)%16:1:3`. It is five
`?:` levels deep, and the comma operator lets some branches also update
counters. The board setup line `S=A<2?f=0,W=COLS/2,...,0:fscanf(...)>3;` uses
the same idea. simple.c uses `if`/`else if` chains.

6. Looking characters up in a string

`"_*!..12345678"[k]` turns the cell type number into the character to draw:
1 is a mine `*`, 2 a flag `!`, 3 a covered cell, 4 an empty cell, and 5 to 12
the digits 1 to 8. The same number `k` is also the color pair. The string is
split into `""` and `"_*!..12345678"` across lines (adjacent strings are joined
by the compiler) to fit the picture.

7. Faked use of a return value

`fscanf(...)>3` can never be true because `fscanf` reads at most 3 values. Its
result (always 0) is stored in `S` and then `S+=W*H` is done. This only stops the
compiler from warning that the return value of `fscanf` is ignored (see the
README). simple.c puts it in an `if` with an empty body and a comment.

8. One array used for two tables

`M` is given room for `2*S` ints. Before the first click the first half holds a
random order (a shuffle, or the priorities from the map file). On the first
click `M[S+M[i]]=i` stores the reverse lookup in the second half, and the first
half becomes the cell states, with a mine wherever the priority was below `C`.
The solver later visits cells in this order through `M[X+S]`. simple.c uses two
arrays, `cells` and `scan_order`.

9. The sign of the timer stores its state

`T=-time(0)` saves the start time as a negative number. While it is negative the
display shows `T+time(0)`, which is the elapsed time. Adding `time(0)` when the
game is lost or won turns it into a positive, fixed number of seconds, and that
also stops the clock. No separate "running" flag is needed.

10. Color table built with index arithmetic

`init_pair(X+1,B[X&&X<10?X-1:2],B[X?X<3?2:1:0])` builds 12 color pairs from a
10 entry array, with ternaries that choose the foreground and background colors.
The global `X` is used as the loop counter here and later as the solver
position. simple.c lists each pair in a table with a comment on what uses it.

11. Work hidden in `for` headers

The display loop's real work (`o+=k==3,Y(k)...,G),p++`) is in the increment part
of the `for`, while the body only sets the character. The mark clearing loop
`for(p=0;p<S&&E==1;M[p++]&=273){}` has an empty body. The game loop
`for(clear();D(),c=getch(),c-'r'&&(c-KEY_RESIZE||E);)` draws, reads a key and
tests it inside the condition. `c-'r'` means `c!='r'`.

12. `goto` out of nested loops

As soon as `K()` changes a cell, `goto N` jumps out of three loops so that the
board is redrawn after each step. simple.c puts the solver in its own function
and uses `return`.

13. Short and reused names

Everything is a one letter global (`M,W,H,S,C,E,X,T,c,p,q,i,j,k`). `typedef int I`
hides the type and `main(I A,char**V)` renames `argc` and `argv`. `c` holds the
key pressed and also the sum from `F(p)`. `n` in `K()` is first a count and
then a cell index. `M[c=1,n]` hides an assignment inside an array index. The
array `U[256]` is never used. simple.c uses descriptive local names.

14. Layout used to hide code

The source is drawn into a shape. Decorative comments (`/*...Semi-Automatic.*/`)
sit in the middle of macros. Backslash-newline is used to split names like
`start\_color` and `BUTTO\N1_RELEASED` across lines. `#include/**/<time.h>`
puts a comment between `include` and the header, which also hides it from
simple text searches.
