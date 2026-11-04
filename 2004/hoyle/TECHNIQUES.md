# 2004/hoyle

Difficulty: Medium

1. Keywords redefined as macros

`void`, `true`, `false`, `switch`, `case` and `do` are all macros, so they do
not mean what they normally mean in C. `void` is `x,x`, `switch(bool)` starts a
`for` loop (`;for(;x<bool;`), `case(a,b)` is the end of a `?:` expression
(`a[O]:b[O]:`) and `do(a,b)` is a comparison followed by `?`. Even the macro
parameter names are keywords (`break`, `default`, `if`, `else`, `bool`), which
the preprocessor allows because it only sees them as plain names.

2. Token pasting to build variable names

`do(if,else)` expands to `inIine(else)>int##if?`. The `##` glues `int` and the
first argument together, so `do(1,x)` becomes `inIine(x)>int1?` and `do(0,x)`
becomes `inIine(x)>int0?`. The variables `int0` and `int1` (the top and bottom
of the current row) look like type names but are `double` variables.

3. Look-alike names

Names are chosen to look like keywords or library names: `doubIe`, `inIine`,
`Iong`, `booI` and `eIse` use a capital `I` instead of a lowercase `l`. `doubIe`
is the coefficient array, `inIine` evaluates the polynomial, `Iong` is the
largest value, `booI` is `argc` (and later the row number) and `eIse` is `argv`
(and also the parameter of `inIine`). `O`, `O1O` and `I` mix the letter O with
the digit 0 and the letter I with the digit 1.

4. Hidden declarations in a macro

`_` is `;double`, so the first line `char*O="..."_ doubIe[010]_ int0,int1 _
Iong=0 _ inIine(...)` really ends the `char*` declaration and declares four more
`double` globals and a function returning `double`, all on what looks like one
statement.

5. Comma expressions that move and restore a variable

`true` is `(--x,x++)` and `false` is `(++x,x--)`. The first one subtracts one,
then gives back that value while adding one again, so `inIine(true)` is the
value at `x-1` and `x` is unchanged afterwards. `false` does the same for
`x+1`. These look at the left and right neighbours of the point to choose `\`,
`/`, `V`, `^` or `_`.

6. Characters stored as offsets in a string

The string `O` is `" <60>!?\\\n"`. Most characters are found by indexing it
backwards (`2[O]` means `O[2]`) and adding `*O` (a space, 32). For example
`1[O]+*O` is `'<'+32`, which is `'\\'`, and `(6[O]-3[O])+*O` is `'?'-'0'+32`,
which is `'/'`. The same string also gives the numbers: `*O` is 32 (half the
width), `5[O]` is 33 (loop end), `-*O>>1` is -16 (first row) and `0[O]` is the
32 rows used for the row height. `!O` is used as the number 0 and `010` is the
octal number 8.

7. One giant nested conditional

The character to print is chosen by a single `putchar` call holding about ten
nested `?:` operators, mostly written through the `do` and `case` macros. In
`simple.c` it is split into the `curve_char` function and an `if`/`else`
chain for the axes.

8. Recursion through main instead of loops

`main` calls itself. The first call (with a real `argv`) reads the
coefficients and finds the scale, then calls `main(-16, 0)`. Because `argv`
is now null, the other branch runs and draws one row, then calls
`main(row + 1, 0)` until the row reaches 16. `simple.c` uses an ordinary
`for` loop over the rows.
