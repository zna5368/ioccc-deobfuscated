# 2013/morgan2

Difficulty: Medium

1. Source code shaped like a picture

The file is laid out in blocks of equal width separated by wide gaps, like the
squares of a domino. Line breaks and spaces go wherever the shape needs them, so
`XDrawRectangle` calls, `for` loops and even a function's braces are cut apart in
random places and nothing is indented to show structure.

2. One letter names, reused

Every variable and function is one letter, and the same letter does several jobs.
`e` is the height of a frame in `a`, the domino to draw in `d`, a neighbor in the
`K` check and the length of the status text in `R`. `p` is the `argc` parameter of
`main`, but it is never used as one. It holds a board position instead. `simple.c`
gives each job its own name, like `selected` or `status_length`.

3. No parameters, only globals

`a` and `d` take no arguments. The caller sets `k`, `t` and `e` before `a()`, or
`x`, `y`, `e` and `t` before `d()`, so a call like `d()` tells the reader nothing.
You have to look back through the code to find which values it will see.
`simple.c` passes them as parameters, as in `draw_frame(left, width, height)`.

4. Depending on leftover values

Some values are never written where they are used. In `==k`, `k` must be 4, but
that is only true because the `for` loop in the last `d()` call stopped when `k`
reached `t`, and `t` was 4. The `t` in the `K` calls is that same leftover 4. `k=!x`
looks like it depends on `x`, yet `x` is never 0 there, so it is just `k=0`.

5. Encoded string

`"tdpsf\73!" "&e!&e!&e\1"` is unreadable because every character is one too high.
The loop `for(...;(*Y++)--;);` in `main` subtracts one from each character at
startup, which gives the format string `"score: %d %d %d"`.

6. Numbers stored as characters

`")%##!!!!"[--q]-33` is the bonus table. Taking 33 away from each character gives
8, 4, 2, 2, 0, 0, 0, 0, and `q` picks one by the number of pending dominoes.
`simple.c` writes this as the array `stack_bonus`.

7. Pointer arithmetic on a string

`5*x+"#fff" "\0#f00\0#0f0\0" "#000"` adds a number to a string. Each color name
takes up 5 bytes with its hidden end marker, so adding `5*x` jumps to color number
`x`. The loop reads as if it were doing math, but it is walking a list of 4 names.

8. Two displays mixed together

The curses and X11 versions share every function. `X` is 0 or 1 and is used to
choose at each step: `((X=...)?P:Q)()` picks a setup function, `(X?R:S)()` picks an
input function, and `d` uses `X?F(...):mvaddch(...)`. `u=X+(1+5*X)*(...)` is a
formula that gives a text column when `X` is 0 and a pixel position when `X` is 1.
`simple.c` splits these into `init_x11`, `init_curses`, `read_key_x11` and so on.

9. Macros that build names

`##` glues text together while the code is compiled. `I(1, RED)` becomes
`init_pair(1,COLOR_RED,COLOR_RED)`, `H(LCORNER)` becomes a test ending in
`ACS_LLCORNER` or `ACS_ULCORNER`, and `J(Up,UP)` builds both `KEY_UP` and `XK_Up`.
A search for those names finds nothing in the source.

10. Array made on the spot

`J` expands to `j==X[(int[]){KEY_UP,XK_Up}]`. This creates a two element array in
the middle of an expression and uses `X` as the index, so it is a hidden choice
between the curses key code and the X11 key code.

11. Whole game logic in a single expression

The third part of the `for` in `main` is meant to be a small step such as `i++`.
Here it is one huge chain of `?:` that handles every key. It is hard to spot
because the code that runs after each key press comes before the drawing code,
which is the loop body. `simple.c` uses `handle_key` and a small function per key.

12. Filler values in conditionals

`?:` needs a value on both sides, even when there is nothing to do. The chain uses
whatever is at hand: `h?h--:1`, `w?w--:i`, `h^7?h++:r`, `g?g--:3` and `:X:k`. These
look meaningful, but they are computed and thrown away, which is why the compiler
warns that a value is not used.

13. Operators used in place of comparisons

`j^113`, `k^64`, `y^e` and `++x^4` use XOR, which gives 0 only when both sides are
the same, so `^` acts as `!=`. `~e` is 0 only when `e` is -1. `113` is the letter
`q`, and `88` is the letter `X`.

14. Multiplying instead of branching

`XLookupKeysym(&E.xkey,0)*!(E.type^KeyPress)` returns 0 for any event that is not
a key press, without an `if`. In `K`, `!!(w)*z[p v]` uses the same trick to ignore
a neighbor past the edge of the board. It reads outside the row or array first
and then multiplies that value by 0.

15. Two values packed in one number

Each entry of `z` holds two unrelated things. The low byte is the shuffled order
of the dominoes, and the bits above it hold the domino placed on that square. The
code unpacks them with `z[p]>>8` and `z[n++]&127`. `simple.c` uses two arrays,
`draw_pile` and `board`.

16. Hidden side effect in a macro

`L` looks like a plain variable, but it is `z[p=rand()&63]`, so each use picks a
new random position and changes `p`. `L|=*z<<8` also means "put the first domino
somewhere random", and `g=p&7` right after it needs the `p` set inside the macro.

17. Dominoes stored as bit patterns

A domino number minus one is 6 bits, one per square, which is how all 64 color
combinations are covered. The sides are compared with bit tricks: mask `3` keeps
the left column, mask `21` (binary 010101) keeps the top row, and a shift lines up
the other domino's opposite side.

18. Scoring and checking in one sum

`K` gives 1 for a matching side, 8 for a mismatch and 0 for no neighbor, and all
four results are added into `r`. Then `((r+3)&~3)==k` rounds `r` up to a multiple
of 4 and compares it to the leftover 4. That is only true when `r` is 1 to 4,
meaning at least one neighbor and no mismatch. The same `r` is reused as the
number of matching sides in the score.

19. Work hidden inside an assignment

`p=k=X&&!(F(0,0,460,300,0))` looks like it sets two variables, and it does set
both to 0. Its real purpose is to clear the X11 window as a side effect, which
only happens when `X` is true because `&&` skips the right side otherwise.
`l=~--m` is similar: it leaves placing mode and sets `l` to -1 for "nothing
selected" in one step, and `l=c[w]-!m++` selects a domino and enters placing mode.

20. One function drawing different things

`d` draws dominoes, empty spaces and the cursor. Setting `e=-1` and `t=4` before
the call turns it into the cursor, because `i` becomes 3 and `(i>2?4:1)` spreads
the 4 squares out to the corners of the box. A blank is `e=0`, which makes color 0.

21. Loop that secretly does a second job

The loop that draws the board also draws the stack. `x+=8; e=c[y];` runs for every
square, but `j^7?(void)j:d()` only calls `d` after the last column of a row. The
`x` it leaves behind is the stack column, and the `x+=8` after the loop then lands
on the selected domino box.
