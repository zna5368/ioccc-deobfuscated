# 2012/dlowe

Difficulty: Hard

1. Source shaped like a Game of Life pattern

The code is laid out in blocks of text that form a 14 cell pattern which turns
into a glider after 19 generations. To fit the shape, statements and even
identifiers are split across lines, such as `KeyReleaseM\` and `ask` joined by
a backslash line continuation, and `f\` `read` inside the `R` macro.

2. Macros for bit packed grid access

The 48x48 universe is stored one bit per cell in 288 bytes. `B(x,y)` turns a
cell into a bit number (`x*48+y`), `C(x,y)` makes the bit mask
`1 << bit % 8`, `A(x,y)` reads a cell as 0 or 1 and `S(b)` sets or clears cell
`(d,e)` in the next grid. The `%` is done before `<<` because of operator
precedence, which is easy to misread. `L` is a macro for the double loop over
every cell using the global counters `d` and `e`, so the loops do not look like
loops. In `simple.c` these are the functions `cell_alive` and `set_cell` and
plain `for` loops.

3. Struct members with the same names as globals

`struct { struct { char c[288]; } c, d; int f, g, a, b; } c;` is a global named
`c` with members `c`, `d`, `f`, `g`, `a` and `b`, the same names as the global
`int a,b,d,e,f,s`. So `c.c.c` is the grid bytes, `c.d` is the next grid,
`c.f` is the speed, `c.g` is the frame counter, `c.a` and `c.b` are the scroll
position, while plain `a`, `b`, `d` and `f` are other variables. The inner
grid is wrapped in a struct so a whole array can be copied with `c.c = c.d`.

4. Reused globals and shadowing

The global `a` holds the screen depth during setup, then a neighbour loop
counter, then the horizontal grid shift. `b` is a loop counter and also the
vertical shift. `k` is a key code and also the neighbour count. The function
`h(int a, int b)` has parameters that hide the globals `a` and `b`. The local
`XGCValues W, B` has the same name as the macro `B`, which only works because a
function like macro is not expanded unless it is followed by `(`.

5. Game state encoded as magic numbers

The global `f` is the game mode: 0 quit, 1 flying, 2 splash screen, 3 flying
up, 4 dead and 5 flying down. The flying modes are the odd ones, so `f%2`
means "the game is running" and `a%2&&b%2` in `h` means "switching between
two flying modes, so do not restart". `simple.c` uses an `enum` and an
`is_flying` function.

6. Nested ternary operators for the key handling

The call `h(f, v.type == KeyPress ? ((k = ...) == XK_q ? 0 : (k == XK_Up ?
(f==4?f:3) : ...)) : ...)` is a single expression that decides the new mode
for every key press and key release. It also assigns `k` inside the
condition. In `simple.c` this is split into `mode_after_key_press` and
`mode_after_key_release` with simple `if` statements.

7. Data files read through `stdin`

The macro `R(d)` calls `freopen` on `stdin` and then `fread`s 288 bytes into
the grid, so the splash screen, the death screen and the sprites are all
loaded as Life grids. The sprite grid is then drawn pixel by pixel into a
pixmap: the first 20x20 bits are the cell tile and the next 5 columns are four
5x5 frames of Mr.C, scaled by 4 with `XFillRectangle`.

8. Hidden state in the graphics context

After building the sprites the loop leaves the GC foreground set to the colour
of the last bit, which happens to be white. The later `XFillRectangle` that
clears the back buffer silently relies on this. `simple.c` sets the colour to
white explicitly.

9. Magic numbers with hidden relationships

Many numbers are linked: a cell is 20 pixels, so `159`, `399` and `81` are
the scroll limits just before 8 cells (160 pixels) of scrolling, and `240` is
the middle position the view jumps back to after the grid is shifted by `8`
or `-8` cells. Mr.C is drawn at `100, 248`, which is why the collision check
loops over `c.a+100` to `c.a+120` and `c.b+248` to `c.b+268`. `16666`
microseconds is one frame at 60 frames per second, `30` and `120` frames are
how often Life runs during the game and on the splash screens, and
`c.g/8+1` picks the animation frame of Mr.C. The speed `c.f` starts at 1000
and is divided by 1000, so it works like a fixed point number.

10. Comma operator in the loop condition

`while(usleep(16666), f)` sleeps for one frame and then tests `f`, so the
loop ends when the mode becomes 0 (quit).

11. Undefined increment

`c.g = ++c.g % (f%2 ? 30 : 120)` changes `c.g` twice in one expression, which
is undefined behaviour in C (the Makefile hides the warning with
`-Wno-sequence-point`). Compilers treat it as `c.g = (c.g + 1) % N`, which is
what `simple.c` writes.
