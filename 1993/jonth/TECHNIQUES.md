# 1993/jonth

Difficulty: Hard

1. Macros that hide most of the code

Nearly every line is built out of one- or two-letter macros: `K` is `case`, `R`
is `return 0`, `G` declares `long i,j`, `S(x,y)` is a `for` loop and `I` is
`S(i,10`. Some macros are only half of a statement. `I` leaves the loop
condition open, so `I-6)` becomes `for(i=0; i<10-6; i++)` (a loop over the 4
cells of a piece), and `z`, `Q`, `F` and `s` end in an unclosed `[` so the
code after them finishes the array index. Bigger macros like `E`, `A`, `H` and
`v` hold whole chunks of drawing or collision logic, so you cannot read a
function without expanding them all in your head.

2. Functions glued together by macros

The line `K 12:Y();}R;}d H 19;}M(J,4)}N H 20;}M(T,4)}q(m,w,k){` closes
`lK()`, writes the complete functions `d()` (draw a piece) and `N()` (erase a
piece) through the `H` and `M` macros, and then starts the header of
`q(m,w,k)`, all inside what looks like the body of `lK()`. In the original
submission (`jonth.orig.c`) this text was hidden even further in the macros
`XX` and `XX1` defined on the compiler command line in the `Makefile`.

3. Source text changed by the build

The `Makefile` runs `sed` over the source before compiling, replacing every `@`
with `Window(D,`. So `XMap@p=XCreate@XRoot@j),...` becomes
`XMapWindow(D,p=XCreateWindow(D,XRootWindow(D,j),...`. The `@` is not legal C,
so the file only compiles after this hidden text substitution.

4. Shape table encoded as a string

The 19 tetromino rotations are stored in a string of `*`, `{`, space and `}`
characters. The loop `*_++=(unsigned)(*c+++2)%4` turns each character into a
number from 0 to 3 (`'*'` is 0, `'{'` is 1, `' '` is 2, `'}'` is 3) and writes
them straight into the memory of the `X` array, filling in the (column, row)
pairs of every block of every shape.

5. Rotation table encoded as a string

`"$#&'(%)+,-*/01.3254"[c]-35` looks up which shape comes after shape `c` when
the piece is rotated. Subtracting 35 (the code of `#`) turns each character into
a shape number, for example `'$'` becomes 1 and `'#'` becomes 0, so the I piece
switches between shapes 0 and 1.

6. One array used for many different things

The `long L[99]` array is used as the window attributes for `XCreateWindow`
(`L[1]` is the background colour), as the graphics settings for `XCreateGC`
(`L[2]` is the foreground colour), as the fake event sent by `XSendEvent` (`*L=2`
is the `KeyPress` event type) and as the text buffer for the score that
`sprintf`, `XDrawString` and `puts` use. The `o` array is likewise used as an
`XEvent`, and the `_[4]` array stands in for a `struct itimerval`. The meaning
of each element depends on knowing the memory layout of the X11 structures.

7. One function that does every move

`q(m,w,k)` performs all piece operations, chosen by the number `k`: 1 places
the first piece, 2 moves down, 3 spawns a new piece, 4 rotates, and the
characters `'j'`, `'l'` and `32` (space) move left, move right and drop. Each
case removes the piece from the board, checks the new position, and either puts
the piece back or redraws it in its new spot.

8. Meaningless names and shadowed variables

All names are single letters (`B` is the board, `g` and `l` the piece column
and row, `r` the score, `h` the level, `n` the "timer ticked" flag, `D` the
display, `J` and `T` the black and white drawing contexts). Inside `main` the
`G,n;` declaration creates a new local `n` that hides the global one for part
of the code, so `n=0` means two different things a few lines apart.

9. Timer events disguised as key presses

The `SIGALRM` handler `f()` cannot call the game logic directly, so it sends
the window a fake `KeyPress` event with `XSendEvent`. `lK()` returns 0 for such
events because their `send_event` field is set, and the main loop then makes
the piece fall one row because the flag `n` is set. `n=r++` in the drop code
both scores a point and sets that flag.

10. Unused arguments and return values

Calls such as `Z(P())` and `U(Z(h=O/9))` pass the result of one function as
the argument of another that ignores it. This is just a way to run two
functions in one statement. `return 0+puts(L)` both prints the final score and
ends the program, and `R+1` (`return 0+1`) is a roundabout `return 1`.

11. Dead and misleading code

`I&&!n)n=B[i][21];` after the full-row check computes a value that is never
used, and the spawn code `z+l]=1` adds the row offset `l` a second time, which
does nothing only because `l` was just set to 0.
