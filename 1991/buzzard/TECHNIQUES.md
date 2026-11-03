# 1991/buzzard

Difficulty: Medium

1. Source code that is also the data

The program has no maze stored in a string. It opens its own source file with
`fopen(x-1?a[1]:__FILE__,"r")` and treats every space as a corridor and every
`\` as the exit. The odd gaps in lines such as `Z[   B       +    X      (   f`
and the tab-indented `(abs(y)		>q)` lines are not just layout: they carve the
corridors of the maze. That is why the code contains filler like the
do-nothing `(abs(y)),` in a `for` increment: it is there only to keep the
shape of the maze. Editing the code changes the maze, so simple.c reads the
original `buzzard.c` instead of its own file.

2. Direction arithmetic in a macro

`#define X(s) (!(s&3)-((s&3)==2))` turns a direction number 0 to 3 into the
step along x (1, 0, -1, 0). The same macro is reused with `f+3` to get the step
along y and with `f+2` to get the negated x step, so `m(d,l)` can look at the
square `d` steps ahead and `l` steps to the side with a single array index.
Turning is just `f+=(b=='r')-(b=='l');f&=3;`. simple.c spells these out as
`step_x()`, `step_y()` and `cell(ahead, side)`.

3. Magic perspective formula

`#define P(x) (x?(5-(x))*(6-(x))/2:11)` gives the half height of a wall at
distance `x` (11, 10, 6, 3, 1, 0). The numbers come from a formula instead of
a table, so it is not obvious they describe a picture shrinking into the
distance. simple.c calls it `half_size()`.

4. One loop that walks forward then backward

The drawing loop `for(s=1,d=0;s+3;s-=2){for(;d!=2+3*s;d+=s){...}d-=s;}`
uses the sign `s` both as "left/right side" and as the direction the distance
`d` moves. `d!=2+3*s` stops at 5 when going forward and at -1 when coming
back, and `d` keeps its value between the two passes. A `break` in the forward
pass (hitting the wall ahead) also decides where the backward pass starts.
simple.c first finds how deep the corridor goes, then uses three plain loops:
left side, end wall, right side.

5. Dead and misleading code

`if(d==5)continue;` can never be true because the loop condition already stops
at 5. The string `"\-"` is an invalid escape that the compiler turns into a
plain `-`. `D(0,"\-")` asks for column 0 but, since the cursor is already past
it, simply appends one dash. These look meaningful but do nothing special.

6. Cursor tracking with tabs

`D(p,s)` keeps the current column in `h` and moves to column `p` by printing
tabs (`h>>3<p>>3`, `h=(h+8)&~7`) and then spaces. Because it can only move to
the right, the order in which pieces of a row are printed matters, which hides
how the picture is laid out. simple.c names it `print_at()`.

7. Short, reused and shadowed names

Globals are single letters (`Z` maze, `A`/`B` position, `f` facing, `g`
center column, `h` cursor). `main` declares `int b` for the typed command and
then an inner block declares `char b[256]` for a line of the file, hiding the
outer `b`. `x` and `a` stand for `argc` and `argv`, and `z()` is only a
wrapper around `atoi()`.

8. Old style declarations and implicit types

Functions use K&R parameter lists (`z(q)char*q;`, `D(p,s)char*s;`) and have
no return types, so everything defaults to `int`. `strlen`, `abs` and `atoi`
are used without their headers. simple.c uses ANSI prototypes and explicit
types.

9. Packed cell values and a combined loop condition

`Z[j][i]=(b[i]==' '?1:2*(b[i]==exit))` stores 1 for corridor, 2 for exit and
0 for wall in a single expression. The game loop `W((b=getchar())!=-1&&m(0,0)==1)`
both reads the next command and ends the game once you stand on the exit, and
`continue` inside the `do` body skips straight to that read when you walk into
a wall.
