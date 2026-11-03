# 1993/cmills

Difficulty: Medium

1. Macro that is cut in half across a function call

`#define C XCreateSimpleWindow(d,` leaves the opening parenthesis open, so
`C*e=C D,...` is really two nested `XCreateSimpleWindow(d, ...)` calls. The
closing parentheses come several lines later. You have to count parentheses
across lines and macro boundaries to see which arguments belong to which
call. `M` (`XMapRaised(d,`) is opened in the same way.

2. Assignment macro that copies from a parent

`#define l(n)t[n]=T[n]+` turns `l(0)x` into `t[0]=T[0]+x`. It copies a value
from the parent record `T` into the new record `t` and adds an offset. The
macro is used inside the argument list of a function call, so each argument
both sets a field and passes its value to X.

3. Raw `long` array used as an array of structures

All of the piece data is kept in one `long z[7992]`, with 8 numbers per
piece: position, size, velocity and the offset of the screen image inside the
window. The fields are only ever reached through `T[0]` to `T[7]`, and moving
to the next piece is `T+=8`. Without names it is hard to tell that `T[4]` is a
horizontal speed and `T[6]` is an image offset. The window handles are kept in
a separate array `w` that is walked in step with `T`.

4. Edge bouncing hidden in a macro and a chain of `||`

`F(n,o,s)` expands to `o+T[n-4] s -0&&++j&&T[n]s!1&&(T[n]=0-T[n])||`. The
four uses of `F` build one long condition that checks the left, right, top
and bottom screen edges. `!1` is just `0` and `-0` is `0`. `++j` counts the
edges that were hit, and `T[n]=0-T[n]` reverses the speed. Because the
reversed speed is not zero the `||` stops the chain early, so only one bounce
can happen per step. The final `||1` makes the whole test always true so that
only the side effects matter.

5. Side effects hidden inside a long `if` condition

The `if` that decides whether a window shatters also assigns
`j=T[2]/2` and `k=T[3]/2` (half the width and height) and calls `rand()`.
`j` first holds the number of edges hit and is then reused for the half
width, all in the same expression.

6. Reused variables

`D` is the root window at first and then becomes the black full screen cover
window. `n` is the screen number and later the loop counter. `c` is `argc`
and then becomes the speed. `b` is the black pixel colour used for both the
cover and the window borders.

7. Ternary on `argc` with XOR

`c=c^1?atoi(g[1]):9` uses `c^1` instead of `c!=1` to check whether an
argument was given, and stores the speed back into `argc`.

8. Assignments buried in function arguments

Calls like `p=XCreatePixmap(d,D=RootWindow(d,n),W=DisplayWidth(d,n),...)` and
`n=DefaultScreen(d=XOpenDisplay(0))` set several global variables while
passing them as arguments, so the setup of the program is hidden in a few
nested calls.

9. Old style function definitions and implicit `int`

`m(w,h,x,y,u,v)` and `main(c,g)char**g;` use K&R style declarations with no
types, so every parameter is silently an `int` and the reader has to guess
what each one means.

10. Code spread over many short lines

Lines are broken in odd places, such as `M` on one line and `s);` on the
next, so a single statement is split up and statements that belong together
look unrelated.
