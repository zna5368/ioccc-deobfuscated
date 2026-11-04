# 2011/toledo

Difficulty: Insane

1. Model data compressed into whitespace and braces

All the 3D models (cube, pyramid, bullet, tank) and the mountain outline
are stored in the string `o`, which only contains `{`, `}`, space, tab
and `;`. IOCCC size rules do not count whitespace or `{`, `}` and `;`
when they are followed by whitespace, so this data is almost free. The
loop in `main` turns each one- or two-character token into a digit from
0 to 8 (`'{'^*o` gives 0 or 6 for a brace, and the next character adds 0,
1 or 2). Two digits form one number as `low + 9*high - 43`, giving values
from -43 to 37. A `;` means "repeat the value from three places back",
once for every character up to the next tab, which saves space where
points repeat. The simple version contains the decoded numbers as normal
arrays (`cube_model`, `tank_model`, ...).

2. Everything in one big integer array

`k[9304]` holds all the models, the mountains, the scores (`j` is left
pointing just after the decoded data) and a scratch copy of every
exploding object at `381+193*c`. Objects store their model as a plain
number (`x f=147` is the tank, `122` the bullet, `0` or `73` an
obstacle) instead of a pointer, so nothing tells you what these numbers
mean. The simple version uses separate arrays and pointers.

3. Single letter names and macros that hide syntax

`#define R while(`, `#define S if(`, `#define I XDrawLine(W,X,Y,` and
`#define n *h++` hide keywords and function calls. `#define x _ c].` and
`#define _ _[` make `x u` mean `_[c].u`: the array of objects is called
`_` and its fields are single letters (`c` hits, `o` color, `m` timer,
`f` model, `u`/`r` position, `s` heading, `i` kind, `e` bullet count or
owner). The Makefile adds `Q` for `XFillRectangle(W,X,Y,`.

4. Key bindings passed as half of a `?:` expression

The Makefile defines `-DA=XK_Up:XK_w`, so `L[c?A]` becomes
`L[c?XK_Up:XK_w]`: the key of player 2 if `c` is 1, otherwise the key of
player 1. The source never shows a complete conditional expression.

5. Loops written as `while` with comma expressions

Almost every loop is `while(a, b, c, condition);` with an empty body, and
the work is done by the comma-separated expressions in the condition.
The loop counters are checked with tricks like `20>++c`, `2>c++` or
`+c--&&...`, so the exact range (11..19, 0..2, 11..0) is hard to see.
`?:` is used instead of `if`, often nested, for example the whole bullet
update is one `?:` chain inside a `while` condition.

6. Global variables reused as temporaries

`u`, `p`, `t` are the input and output of the rotation function `K()`,
and `d`, `e`, `f`, `i`, `q`, `h` are reused for unrelated things in
different functions (a screen coordinate, a random offset, a collision
distance, a loop counter...). `c` is the "current object" used by `g()`,
`G()` and `T()`, and is even changed in the middle of firing a bullet
(`c=d; ... c=x e;`) so that `g(19)` moves the bullet instead of the tank.
Function parameters `N` and `i` in `K(N,i)` hide the globals of the same
name.

7. Extra and ignored function arguments

Old-style functions accept any number of arguments, so color changes are
passed as unused extra arguments: `s(..., x f, b(x o))` calls `b()` only
to set the color before drawing, `b(0, d=20*x c)` sneaks an assignment
in, and `c[b(x o),j]` calls `b()` and then reads `j[c]` (same as `c[j]`).
`K(o,+p+w,u+=g)` uses its third argument only for the side effect on `u`.

8. Magic numbers

Colors are written as `249<<16-c*8` (red/green), `64987<<8*(c>2)`
(cyan/yellow), `4092<<12` (orange) and `(1<<16)-1`. `20` in `XChangeGC`
is `GCForeground|GCLineWidth`, `3` in `XSelectInput` is
`KeyPressMask|KeyReleaseMask`, `w=11.46` is 72/(2*pi) so that 72 heading
steps make a full turn, and `-+-+-+-+147` is just `147`.

9. Fixed point positions and integer 3D math

Positions are stored multiplied by 256 (`<<8`) and divided back with
`>>8` when drawing. Rotation results are cut down to integers, and the
screen projection `u*N/d+M/2` divides by depth with a minimum of 1. The
clamp to `±2*M` is written as nested `?:`.

10. Hidden tricks in calls and expressions

`XOpenDisplay(k)` passes the empty int array as an empty string so the
default display is used. `XLookupKeysym(&e, XNextEvent(W,&e)&0)` reads
the next event inside the argument list and turns its return value into
the index `0`. `L[...]^=1==e.type/2` flips a key on both press (2) and
release (3) because both divide to 1. `a=++a%72` counts the splash
screen frames and `P-=o<2` ends the level when fewer than two things
are still alive.

11. Missing types and headers

Functions and globals have no types (implicit `int`), `cos`, `sin`,
`rand`, `abs`, `time` and `usleep` are used without their headers, and
`main` never returns. `L[1<<28]` is a 1 GB array so that any keysym can
be used as an index without a lookup.

12. Code shaped as a cube

The source is laid out as a picture of a cube, so line breaks fall in
the middle of expressions and names, hiding the structure of the code.
