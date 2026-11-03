# 1998/banks

Difficulty: Insane

1. Source shaped like a picture

The whole program is laid out in the shape of an aeroplane. Statements are split
in the middle of names and numbers (for example `X*4.9/l`, or `"%5d  %3d"` and
`"%7d"` sitting on separate lines). C does not care about line breaks or extra
spaces, so the shape hides where each statement starts and ends.

2. Single-letter and meaningless variable names

Every global is one letter: `L`, `c` and `i` are the plane's x, y and altitude.
`l`, `M` and `X` are its forward, sideways and downward speeds. `o`, `j` and `O`
are roll, pitch and heading, and `v`, `d` and `F` are how fast each one changes.
`h`, `J` and `u` are the throttle, elevator and aileron. Even the time step is
called `_`. Nothing in the names says what the values mean.

3. Reusing the same variable for unrelated things

There are only a few temporary variables, so each one holds many things during a
frame. `T` is first the sine of the heading, then the height of a scenery point
above the plane, then the squared speed. `D`, `E`, `K`, `W`, `I`, `m`, `t`,
`a`, `H` and `P` each hold two to four different quantities as well. `H` and `P`
are worse: they hold an acceleration at the end of the loop, that value is used
at the top of the next frame, and then they are overwritten with a
rotation-matrix entry. To read the code you have to keep track of which meaning
each variable has at every line.

4. Physics hidden in bare numeric constants

The flight model uses numbers like `74.5` (mass in slugs), `32.2` (gravity),
`4.9`, `.19`, `.64`, `125e2`, `107e2` and `2312` (lift and damping
coefficients) with no names. Some are written in unusual forms, such as `4E2` for
400, `1E3` for 1000 and the octal `0550` for 360. The `% 0550` keeps the heading
between 0 and 359 degrees.

5. Interleaved and reordered calculations

The rotation-matrix entries (`m`, `P`, `t`, `H`, `I`, `a`, `r`, `Z`) and the
updates of the three angles are mixed together. `XClearWindow` is called halfway
through them. Some values are computed inside a `for` loop's start or step
section, such as `o += (I = ..., ...)` and `T = sin(O)`. Others are computed
inside the arguments of `sprintf` (`p = l/1.7`, `C = 9E3 + O*57.3`) or `select`
(`select(p = 0, ...)` resets the scenery loop counter). The reader has to
work out what depends on what before the code can be put back in a sensible
order.

6. Comma operator to hide assignments

Expressions like `(E = .1 + X*4.9/l, t = T*m/32 - I*T/24) / S` and
`(I = M/l, l*H + I*M + a*X)` do one assignment and then use the last value. The
comma operator evaluates everything from left to right and keeps only the last
result, so assignments to `E` and `I` are hidden inside formulas. A side effect
is that the y-position update uses `I = M/l` instead of a rotation-matrix entry.
simple.c keeps that quirk so the output stays the same.

7. Pointer-selecting ternaries for keyboard input

`++*(N - IT ? N - LT ? UP - N ? &E : &J : &u : &h)` chooses which variable to
add one to (throttle, aileron or elevator) based on the key pressed. If the key
is not a control key, it adds to the unused temporary `E` instead. The matching
`--*(...)` statement subtracts one in the same way and uses `W` as its unused
target. The loop step `u *= CS != N` multiplies the aileron by 0 when the
"center stick" key is pressed. All of this is a chain of `if`/`else`
statements in disguise.

8. Array indexing written backwards and in pointer form

`p[s]`, `n[p]`, `w[p]` and `p [n]` mix the two spellings of array access
(`a[i]` is the same as `i[a]`). `scanf` is given `y + n`, `w + y` and `y + s`
instead of `&n[y]`. The `scanf(...)+1` loop condition is a hidden way of writing
"until `scanf` returns `EOF` (-1)".

9. Sentinel value and bitwise operators instead of logic

`N = 1e4` (10000) is a "pen up" marker. It means there is no previous screen
point, so the next point must not be joined by a line. `N - 1E4 && XDrawLine(...)`
uses `&&` as a short `if`. The visibility test uses `|` instead of `||` so that
the assignments to `W` and `D` inside it always happen.

10. Configuration through compiler macros

The keys (`IT`, `DT`, `UP`, `DN`, `LT`, `RT`, `CS`) and the time step `dt` are
not in the source. They are given on the compiler command line with `-D`, so
the source alone does not show which keys or which frame time are used.
