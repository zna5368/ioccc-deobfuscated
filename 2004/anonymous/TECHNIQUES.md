# 2004/anonymous

Difficulty: Insane

1. Whole program in one `for` loop driven by a state variable

`main` has no body. Everything happens in the third clause of
`for(...; C-1; ...)` as one huge expression made of nested `?:` and `,`
operators. `argc` (`C`) is reused as a state number: 2 reads the next
letter, 3 draws the letter, 4 prints the image, and 1 stops the loop. Each
pass of the loop does one small step and then changes `C` to pick what
happens next. In `simple.c` each state became an ordinary loop or function.

2. Meaningless single letter names, including `_`

The globals `b, l, k, o, B, _, Q, s, V, R, T, O, L, x, X, i, z, Z` give no
hint of their job. `_` is both a macro name (`_(i,j)`) and a `double`
variable (the spline parameter `t`). `Q` is a line step counter in one state
and the x pixel position in another. `C` is `argc` and the state number.

3. Macros that hide repeated code and odd bracket placement

`yy(i)`/`y` shift the four saved pen positions, `q(Q,R)` decodes three
characters into a point, `_(i,j)` computes spline coefficients and `v(i)`
evaluates the spline. `Y` calls `_` as `_ (0 ], 4] )`, so the closing `]`
brackets are passed inside the macro arguments and the macro body supplies
the opening `[`. This makes the code impossible to read until it is run
through the preprocessor.

4. Parallel arrays in place of a 2D array

`R`, `T`, `O`, `L` are four arrays of 8 `int`s. Index 4 to 7 hold the four
most recent pen positions (oldest in `R`, newest in `L`) and index 0 to 3
hold the cubic coefficients of the curve. In `simple.c` these are
`point[4][COORDS]` and `coef[4][COORDS]`.

5. Encoded vector font hidden in a long string

All letters live in one string `Z`. Each letter starts with `4` plus the
letter, and is found with `strstr(Z, "4c")`. Points are stored as three
printable characters whose low bits are combined with `%`, `/`, `&` and `|`
(`y = c0%64*8 | c1/8&7`, `x = c1%8*64 + c2%64 - 256`). Command characters
are told apart only by comparing with numbers (`*X>60` is a point, `>57` is
`:` end of stroke, `>54` is `8` move the pen, anything else ends the letter).
`simple.c` keeps the encoded strokes but puts each letter in a table entry
and explains the format.

6. Shared string pieces glued with string literal concatenation

Parts that several letters use (stems and bows of the tengwar) are macros
`w, r, S, t, p, W, U, u` that are pasted together next to each other, e.g.
`"4d"W t t` builds the letter `d` from a stem and two bows. The macros are
defined in a scattered, oddly indented order so they look like decoration.
In `simple.c` they have names like `TALL_STEM` and `OPEN_BOW`.

7. Magic numbers and character codes

`104` is `'h'`, `^=32` turns a letter into upper case to pick the digraph
glyph, `163840` is 256 columns of 640 pixels per input letter, `.5/256` is
the step of `1/512` along each curve, `16` is the number of dots drawn
across the pen nib, and `61` is `'='`.

8. Catmull-Rom spline and broad pen hidden in arithmetic

The coefficients `2*T`, `O-R`, `2*R-5*T+4*O-L`, `3*T-R-3*O+L` are the
Catmull-Rom spline formula, written with no explanation. Each pen position
has two points (the two edges of a calligraphy nib), and for every curve
sample a short line of 16 dots is drawn between the edges to fill the stroke.

9. Short circuit operators used as `if` statements

`(*x++==104?z[1]^=32:--x)` reads one character ahead and puts it back with
`--x` if it is not `h`. `*X-61||(...)` runs the bracketed code only when the
character is `=`. In the output state `printf(...)&&(Q+=2)<V||(Q=0,s+=2)<640||(C=1)`
uses the return value of `printf` (always true) so the column, row and stop
logic can follow in the same expression.

10. Hidden quirk from the string search

Because a letter is looked up with `strstr`, a space followed by `h` becomes
the key 0 (`' '^32`), and searching for `"4"` finds the first glyph (a
comma). `simple.c` keeps this behaviour in `find_glyph`.

11. Header hidden by a line continuation

The first line is `#include\` with `<stdio.h>` on the next line, so the
include is split across two lines by a backslash.
