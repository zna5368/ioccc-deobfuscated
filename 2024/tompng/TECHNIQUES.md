# 2024/tompng

Difficulty: Hard

1. Code hidden among comment "noise"

The source is drawn as a swirling vortex. Most of the picture is made of
random-looking letters like `N/+ + + / n /` that sit inside `/* ... */`
comments, and the real code is split into tiny pieces between them. For
example `/*  */long/*  / n / ** T  +C C 5  , a\n ... */long/*  ...  */N,M,`
is only `long long N,M,` once the comments are removed. Even keywords and
expressions are cut in half, such as `B/* ... */[(y+d*j*w)/*N*/%N]`. Running
the file through the preprocessor (which deletes comments) shows the real
program.

2. Tricky comment endings

Comments like `/***/` and `/**/` look like operators but are complete
comments. In `d*j/***/*l` the `/***/` is an empty comment, so the code is
`d*j*l`. The reader has to count stars and slashes carefully to see where
each comment starts and ends.

3. Code placed before the `#include` lines

The globals and the whole stirring function `Q` come before `#include
<stdio.h>`. Because `Q` only does arithmetic and array copies, it needs
nothing from the headers, but it surprises a reader who expects includes at
the top.

4. Single-letter names and reused variables

All globals are single capitals: `N` (rows), `M` (columns), `S` (seed from
the text), `T` (password hash), `W` and `X` (hash constants), `A` (the text
grid) and `B` (a copy of it). In `main`, `x` is first the column while
reading input, then the direction (+1/-1), then the animation frame number;
`c` is first the character read, then the difference between two scores.
The rewrite gives each job its own descriptive name.

5. Constants built from expressions

Directions and table values are computed instead of written down. In `main`,
`x=-(6>>i/32)%2*2+1` makes `+1, -1, -1, +1` for the four quarters of the
128-step loop. In `Q`, `k=~d/2*3+3` picks the first line type (0 or 3) and
`(3+5*d)/2` picks where to stop (4 or -1). The column step for each line type
is `(2*k-k/2*3-k/3*5)*d`, which gives `0, 2, 1, -2`, and the wave strength
`3+k%3+k/2` gives `3, 4, 6, 4`. The rewrite stores these as small tables
(`COLUMN_STEP`, `WAVE_STRENGTH`) and named directions.

6. Loops with empty bodies

Most `for` loops have all their work in the third (increment) part and an
empty body. For example, the wave loop
`for(w=0,l=6;l-=2;s=(s+T)*W%X,x=...,w+=...);` runs twice (with `l` equal to 4
and then 2) because the condition `l-=2` both changes `l` and tests it. The
rewrite uses an ordinary `for (frequency = 4; frequency >= 2; frequency -= 2)`
loop with the statements inside the body.

7. Condition and assignment merged in a `for` header

In `for(int i,j,k=...,l;j=k!=2,k-(3+5*d)/2;k+=d)` the condition part first
sets `j` (whether the line is vertical/diagonal) with the comma operator and
then tests whether `k` has reached the end. Hiding a side effect inside the
loop condition is easy to miss.

8. One loop doing two different jobs

The 128-step loop in `main` runs the stirring forward, back, back again and
forward again. Only at steps 31 and 95 (`i%64==31`) does the inner loop
actually run, to measure how "tidy" the text looks. Each loop in the rewrite
does just one of these things.

9. Operator precedence traps

The tidiness score adds `a|!b&&(j%M)`. Because `|` binds tighter than `&&`,
this means `(a | !b) && (j%M)`: count a cell when it is not in the first
column and does not start a word. It looks like it adds the value of `a`,
which is wrong. The rewrite spells out the test with named variables.

10. Ternary operators used as statements

`c-10?A[y][x++]=c:(y++,x=0)` replaces an `if`/`else` that stores a character
or starts a new line. `!j?!fprintf(...):puts(...)` chooses between drawing
the animation on standard error and printing the result on standard output.
`c<0?Q(31-x,-1):Q(x,1)` picks decrypting or encrypting. Each becomes a plain
`if` in the rewrite.

11. Flattened two-dimensional loops

Loops like `for(i=M*N;i--;)B[i%N][i/N]=A[i%N][i/N];` use one counter for
the whole grid and split it into row (`i%N`) and column (`i/N`) with
division. The rewrite uses two nested loops over rows and columns.

12. Split and hidden string literals

`"%l" "ld"` is joined by the compiler into `"%lld"`, and `"\033" "[H"`
becomes the "cursor home" escape. `x?"%s":"%s\033[2J"` picks a format that
also clears the screen on the first frame. The delay `usleep(W<<1)` reuses
the hash constant 33331 to make 66662 microseconds.

13. Adding big multiples to avoid negative remainders

In `Q`, `m*4` (four times the grid size) is added to every coordinate, and
`n*(n+s%W)` is added to the wave position. This keeps the numbers positive,
so `%` always gives a proper wrap-around index. It looks like part of the
formula but is only a safety offset.
