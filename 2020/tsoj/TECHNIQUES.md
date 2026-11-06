# 2020/tsoj

Difficulty: Medium

1. Right-justified reverse indentation

Every line is pushed to the right so that it ends near the same column, and
lines that are nested deeper are often less indented than the lines around
them (for example `L:` and `K:` sit far to the right). The shape of the code
tells you nothing about which statements belong to which block, so you have to
follow the braces and semicolons yourself.

2. Single-letter names that reuse the same letter

Every letter is used as a name: `t` is the clock function, `g` is both a macro
(draw a sprite) and an `int` variable (first frame flag), `l` is both a macro
(box collision test) and an `int` variable (last frame time), and `L` is both a
label and an `int`. A function-like macro is only expanded when its name is
followed by `(`, so `g(...)` draws a sprite while `g=0` sets the variable.
Inside `main`, `t` is also the name of the `struct timeval` in `t()`, and `z`
is used for both the termios settings and an asteroid velocity pointer.

3. Macros instead of functions

`g(x,y,u,v,s)` is a whole `for` loop that `memcpy`s each row of a sprite into
the screen buffer, and it secretly uses the variables `h`, `w` and `f` from
`main`. `l(x,y,w,h,a,b,c,d)` is a long boolean expression that tests whether
two rectangles overlap; its parameters `w` and `h` hide the screen width and
height while it is expanded. In simple.c these are the `draw_sprite` and
`overlaps` functions.

4. `goto` loops

`K: ... if(i--)goto K;` and `L: ... if(i--)goto L;` are loops written by hand.
`i` starts at the last index and the jump repeats the body until `i` reaches
zero, which is the same as a `for` loop counting down to 0.

5. No `==` or `!=`

Comparisons are replaced with other operators. `while(u^'q')` uses XOR, which
is non-zero when `u` is not `'q'`. The read loop condition
`(u&'p')>=(u|'p')` is only true when `u` equals `'p'` (AND and OR give the same
value only when no bits differ), or when `u` is a negative `char` (its OR has
the sign bit set), so it means "keep waiting while paused".

6. Nested `?:` chains instead of `if`/`switch`

The movement keys are handled by `q^'a'?q^'d'?q^'w'?q^'s'?0:(...):(...):...`.
Each `q^'x'` is false only for that key, so the innermost matching branch runs.
Asteroid respawning, firing and game over are also written as
`condition ? a, b, c : 0` expressions instead of `if` statements, with the
comma operator doing several assignments in a row.

7. Packed string literals for graphics

`r=" /\\ /__\\ ^^ "` is the 4x3 ship and `n="o0o0OO"` is the 3x2 asteroid,
stored as one flat string with no line breaks. The sprite macro cuts it into
rows using the width (`&s[j*u]`). Characters such as `32` (space), `46` (`.`)
and `64` (`@`, the bullet) are written as numbers.

8. Treating a 2D array as 1D

The screen `f[h+1][w]` is filled with `(*f)[i]`, which takes the first row and
indexes past its end into the following rows. The star field formula
`L%3+L%5+...+L%19>14` produces a pseudo-random pattern of dots from the cell
index and a growing counter, so no star positions are stored.

9. Many declarations crammed into one line

`int h=v.ws_row,w=v.ws_col,A=w*h/100,l=t(),k,s=0,g=1,i,c=0,L;` sets up the
screen size, asteroid count, timer, score and flags in one statement, and
`struct V{...}p={w/2,h/2},b={-3*h},a[A],m[A];` declares the ship, bullet and
all asteroid positions and velocities together. Only the first member of `b`
is given, so the bullet starts far off screen at `x=-3*h`.
