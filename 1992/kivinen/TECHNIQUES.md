# 1992/kivinen

Difficulty: Hard

1. Visual layout with tabs

The `#define` lines are indented with different numbers of tabs and spaces.
The body of `main` is spread out with long runs of tabs that leave holes in
the text, so the source looks like a picture. The compiler ignores this
whitespace, so it only makes the code hard to read.

2. Short macro names hiding X11 calls

`A` is `XCreateSimpleWindow`, `R` is `BlackPixel(d,D)`, `D` is
`DefaultScreen(d)`, `E` is `r.xbutton.button`, `S` is `r.xvisibility`, and
`_` and `V` are event masks. `X(x,v,y)` moves window number `x` and multiplies
the position by the scale `z`. To read the code you have to keep swapping each
letter for the long X11 name it stands for.

3. Macro with a hidden side effect

`N(v)` expands to `(XMapWindow(d,s[v]),v)`. It shows a window and then
returns its index. The argument appears twice in the expansion, so
`N(c--)` really runs `c--` twice. This silently turns the window count (32 or
52) into the number of targets (30 or 50).

4. `exit` redefined as an expression

`#define exit(a) (exit(a),0)` turns `exit` into an expression that has a
value. This lets the program call `exit` inside a `?:` operator, for example
`u>140?exit(c):...` and `s[1]?X(...):exit(++c)`, instead of using an `if`.

5. Behaviour chosen by the argument count

The program never reads its arguments. It only uses their count: the scale is
`z=(a+1)/2`, `x=a&1` picks a game where something falls (invaders or dropout),
and `v=z&1` picks invaders over dropout. One set of code runs three games,
and every line has to be read three times with tests like `x&v` and
`!x||!v`.

6. Reused one letter variables

Each variable means something different depending on the game. `i` and `u`
are the position of the missile, the falling piece or the ball. `k` is the
ball's sideways speed in breakout but an alien's column in invaders. `l` is
the outermost alien column in invaders but a "something was hit" flag in the
other games. `e` is the height of the alien fleet, and its bits also store
which way the fleet is moving and whether it is moving down a row.

7. Control flow written as `?:` and comma expressions

Almost all of the logic is written as nested conditional expressions joined
with commas instead of `if`/`else` blocks. Examples are
`e&31?e++:e&32?(q++,(q+l>300?e++:0)):(q--,q+l<10?e++:0)` for the fleet
movement and
`k>0?i+=(i>105?k= -k:k):(i+=(i<1?k= -k:k))` for the ball bouncing off the
walls.

8. One condition handling every event

Mouse presses, mouse releases and visibility events are all handled inside the
condition of a single `if(!(...))`. Each branch returns a value, and only a
value of zero runs the body that searches for a hit. A visibility event uses
`r.type^VisibilityNotify|S.state^1`, which is zero only for a
"partially covered" event. Pressing button 2 in breakout or dropout sets the
speed `t` to 0, so it also runs the search for a hit.

9. Collision detection with window visibility

Nothing in the code compares coordinates. Every brick, alien, paddle and
missile is its own X window. When one window partly covers another, the X
server sends a `VisibilityNotify` event with state `VisibilityPartiallyObscured`
to the covered window. The program uses that event to detect a hit. It is also
why the ship dies when it leaves the playfield: being clipped by the edge
counts as being partly covered.

10. Bit tricks for constants

`j=~0` is -1 (the ball starts moving up), `u=~19` is -20 (the piece is put
back above the screen), and `c^31` and `w^1` stand in for `c!=31` and `w!=1`.

11. Initialization in odd places

`k=q=1` is written inside the argument list of `XCreateSimpleWindow`, where it
also supplies the border width of 1. `y` and `i` are assigned inside the
arguments of `X(...)` calls, and `c=0` is set inside `N(c=0)`. The paddle
speed `t` is never set before it is first read.

12. Loop index shared between loops

In invaders, the hit search starts with `w++` instead of setting `w=0`, so it
continues from the value left by the fleet loop (`-1`). After the first search
in a frame, `w` is already at the end, so any later visibility events in that
frame are ignored. The loop is also written as `for(...;++w<52-x*20;)`, which
puts the increment in the condition.
