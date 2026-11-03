# 2001/rosten

Difficulty: Medium

1. The file includes itself

The file is wrapped in `#ifdef s ... #else ... #endif`. On the first pass `s`
is not defined yet, so the compiler reads the `#else` half. That half sets up
the headers, macros, globals and `main`, and then defines `s`. At the very end
`#include __FILE__` pulls the same file in again. Now `s` is defined, so the
compiler reads the top half, which holds the function `r()`. This means the
code at the top of the file is really compiled last, after the code below it.

2. Octal digit swapping macros that cancel out

`H(a,b)` takes out octal digit number `b` of `a`, which is a group of 3 bits.
`G(c,d)` swaps octal digits `d` and `d+1`. `s(e)` does six of these swaps in a
row, and together they reverse the lowest four octal digits of a number. So
`s(512)` (octal `1000`) becomes `1` (octal `0001`), which is how the flag `e`
starts as 1. Doing the reversal twice gives back the first number, so
`g=s(DisplayWidth(...)-1)` stores a scrambled width and every later use, such as
`o>=s(g)`, unscrambles it again. Tests like `s(o)<=0` and `s(p)<=0` give the
same answer as `o<=0` and `p<=0`, because the reversal never changes a number's
sign or turns a non-zero number into zero. In `simple.c` the plain width and
height are stored and the macros are gone.

3. Identifiers split with token pasting

`I(aa,bb)` is `aa##bb`, which glues two tokens into one. Calls are written as
`I(XWarpP,ointer)`, `I(Defa,ultScreen)`, `I(Display,Width)`,
`I(DisplayH,eight)` and `I(clo,ck)`, so the names `XWarpPointer`,
`DefaultScreen`, `DisplayWidth`, `DisplayHeight` and `clock` never show up
whole in the source.

4. Macros that hide declarations and the end of a function

`x` turns into `o,p; Window m;`, so the line `z k=0,l=0,n,x` really declares
the pointer coordinates `o` and `p` and the window `m`. `ghj` turns into
`unsigned int*`. `JJ(X)` turns into `));return 0;}X`, which closes the
`XWarpPointer` call, returns from `r()` and adds whatever text it was given.
`JJ(float B;int)C,D;` uses this to finish the function and then declare the
unused globals `B`, `C` and `D`. `z` is a `typedef` for `int`.

5. Control flow written as `&&`, `||` and commas

There are no `if` statements. `(cond)&&(action)` only runs the action when
the condition is true, for example `(X==2)&&(f=atof(Y[1]))` for the optional
argument and `((i=XOpenDisplay(0))==0)&&(exit(1),1)` for the error check.
`(e==1)&&(c=o,d=p,e=0,1)||(...)` is an if/else: on the first call it saves the
position and the `,1` makes the left side true so the `||` part is skipped.
On later calls the `||` part runs. The main loop puts its whole body in the
step part of an empty `for(;;)`.

6. Misleading operator precedence

`l^-1==-1` looks like it compares `l^-1` to `-1`. But `==` binds tighter than
`^`, so it is really `l^(-1==-1)`, which is `l^1`. Because `l` is only ever 0
or 1, this is just `l==0`, the same as the `k==0` test for the other axis.

7. Rounding hidden in a sign macro

`y(X)` is `((X>0)-(X<0))`, which gives the sign of a number (-1, 0 or 1).
`(z)(a+y(a)*.5)` adds or takes away one half before turning the float into an
int, which rounds the speed to the closest whole pixel (halves go away from
zero). In `simple.c` this is the function `round_away()`.

8. Single letter names and code shaped like a picture

Every variable is one letter: `a` and `b` are the pointer speed, `c` and `d`
the last position, `f` the greasiness, `g` and `h` the screen edges, `i` the
display, `j` the screen and `q` the last tick. The code is laid out as a
drawing of a mouse, with comments like `/*Egads! something has taken a huge
bite out of the mouse pointer!!!*/` chopped up to fit the shape. A comment
`/* _XSetPointer(display, screen,GREASY|BOUNCY)*/` is even placed in the middle
of the `G` macro so that it looks like real code.
