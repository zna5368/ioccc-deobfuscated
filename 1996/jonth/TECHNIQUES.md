# 1996/jonth

Difficulty: Hard

1. Program hidden inside the X11 header

The file has no code after the `#include` lines, only macros. The macro
`XIMOfIC` has the same name as a function that `<X11/Xlib.h>` declares with
`extern XIM XIMOfIC(XIC);`. When the header is included, the preprocessor
replaces that name with the whole program, so all the functions get defined in
the middle of the system header. The text `extern XIM` before it becomes the
harmless declaration `extern XIM Z;` and the `(XIC);` after it becomes
`GC *g(XIC);`. Because `math.h` and `Xlib.h` are included after the macros, a
normal `cpp` run with the system headers left out shows no code at all.

2. Macro chain spelling OBFUSCATED

`#define O B F U S C A T E D` is used as an array size in `R[2][O]`. `O`
expands to the letters `B F U S C A T E D`, and most of those letters are
macros themselves (`T` and `D` are plain identifiers: a function and the
current player). Each macro holds a random piece of the program, cut in the
middle of statements, so the pieces only make sense once they are joined in
the order of the word. `B` even begins with `3][3];` to finish the
declaration `R[2][3][3]` before it starts the next declarations.

3. Same bitmap code pasted three times by a macro

`Y(z,y)` expands to a `for` loop that first creates a pixmap from the bitmap
`b`, then clears `b` with `memset`, then loops over 50 rows. Because the
pixmap is made before the clear, each pixmap holds the picture drawn by the
*previous* loop: `G` gets the empty bitmap, `W` gets the cross and `o` gets the
circle. The third loop has nothing left to draw, so its body is just
`XMapRaised(V,e)`, which maps the board window 50 times.

4. Drawing hidden in the parts of a `for` loop

The cross is drawn in `for(i=c.event_mask=4;i--;x+i>H||z(...))x<i||z(...)`.
The loop counter is also used to set the event mask to 4 (`ButtonPressMask`),
the condition `i--` counts 3, 2, 1, 0, and both the "increment" part and the
body draw pixels. `||` acts like an `if`: the right side only runs when the
left side is false, which keeps the lines inside the picture.

5. Order of evaluation tricks

The circle is drawn with
`z(x+k,y=sqrt(l*l-x*x)+k),z(k-x,y)-z(y,x+k)+z(y,k-x)*z(x+k,y=H-y),...`.
The results of `z` are subtracted, added and multiplied only to string the
calls together. In the middle of this, `y=H-y` mirrors `y` to the top half of
the circle, so the eight symmetric points of a circle are plotted with a value
of `y` that changes half way through the expression. The C standard does not
say in which order the operands of `-`, `+` and `*` are evaluated, so the
program relies on the compiler going left to right.

6. One global array for two jobs

The array `b` is the 50x50 bitmap (7 bytes per row) used to build the
pixmaps, and after the setup it is the game board: square `x,y` is
`b[x+k*y]` with `k=25`. `!b[x+k*y]++` checks that the square is free and
marks it taken at the same time. Since `b` is a `char`, clicking an occupied
square 255 more times wraps the count back to 0, which is the cheat the
judges mention.

7. Global variables used as hidden function arguments

`x` and `y` are globals that the loop in `M()` leaves pointing at the clicked
square. `t()` and `r()` never receive the square as a parameter, they read the
globals. `r()` also secretly sets the current display `V` and window `m`, and
`V` is then used by `XNextEvent` to decide whose turn it is.

8. Clever expressions for small decisions

`*(h+=!!*h)` moves to the next command line argument only if the current one
is not `NULL`, so with no arguments both boards use the default display.
`"II"+D` skips the first letter for player 1 to get the window titles "II" and
"I". `t(D),t(!(D^=1))` switches the player while drawing the same mark on both
boards. `main` calls `M` with the result of the two setup calls as an unused
argument, and `M()` calls itself forever instead of using a loop.

9. Function pointer with magic numbers

`XCreateWindow` is only called through the pointer `w`, and its arguments
are plain numbers: `0` for `CopyFromParent`, `1` for `InputOutput`, `2048` for
`CWEventMask`, and `152` for three 50 pixel squares plus their borders.
Functions use implicit `int` return types and old K&R style parameters.
