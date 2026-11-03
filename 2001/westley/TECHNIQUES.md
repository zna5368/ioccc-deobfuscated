# 2001/westley

Difficulty: Medium

1. A macro that changes meaning depending on line order

`t(a,b){return a;}` is a function that returns its first argument. The line
`#define t(a,b) t(b,a)` swaps the two arguments, but the preprocessor only
applies a macro to lines that come *after* the `#define`. So `t(1,0)` gives `1`
if it is above the `#define` and `0` if it is below it. The program has four of
these calls, and each one is a hidden on/off switch:

- `J()`: `t(i,0)` decides if the lines are reversed (above the `#define`
  the loop starts at `i` and does nothing).
- `M()`: `t(1,0)` decides if a line is printed as text or as a punch card.
- `E()`: `t(0,1)` decides if the lines are rearranged at all.
- `F()`: `t(0,1)` decides if lines are compared randomly (scrambled) or with
  `strcmp()` (sorted).

In `simple.c` these switches are the four constants `REVERSE_LINES`,
`PRINT_AS_TEXT`, `REARRANGE_LINES` and `SCRAMBLE_LINES`, set to the values the
original line order gives.

2. Lines that can be shuffled

Every line is a complete function or global declaration, so the lines can be put
in any order and the program still compiles. This works because old C lets you
call a function that has not been declared yet (it is assumed to return `int`),
and because the globals are declared again with no value (`char* d,*m,*B;` and
`char* K;`) on the lines that use them. Repeating a global with no value is a
"tentative definition" and is allowed. The extra `; ; ;` after some lines and
the lines that start with spaces are padding so that `sort` and `sort -b` put
the lines into an order that gives a useful program.

3. Trigraphs

The code uses three character sequences that the compiler turns into one
character (when `-trigraphs` is on): `??<` is `{`, `??>` is `}`, `??(` is `[`,
`??)` is `]` and `??/` is `\`. They hide the braces of every function, the array
sizes `c??(81??)??(81??)` (`c[81][81]`), the punch hole character `'??('`
(`'['`), the newline in `"/%-80s|??/n"` and the escaped question mark at the end
of the `K` string `"...obfuscated??/?"`.

4. Pointer arithmetic instead of array indexing

Instead of `c[i]` the code writes `*(c+i)` everywhere, for example
`gets(*(c+i))` and `*(k+i)!=c`. Both forms mean the same thing, but the long
form is harder to read. The author notes that it is shorter than the trigraph
version `c??(i??)`.

5. Single letter function names

All functions are named with one upper case letter (`A`, `D`, `E`, `F`, `G`,
`H`, `J`, `L`, `M`, `P`, `Q`, `R`, `S`, `T`, `U`, `V`, `W`, `X`, `Y`, `Z`) and
are written in a random looking order. Many of them are tiny wrappers, such as
`D()` and `L()` which just pass two rows of the array on to `E()` and `G()`.

6. Extra parameters used as local variables

Functions are declared with more parameters than they are called with, such as
`J(i,c,j)`, `A(i,c,j,k)`, `S(c,r,i)`, `V(c,k,i)` and `Q(t,i)`. The extra
parameters are used as local variables, which saves writing a declaration. Even
`main(i)` only has one parameter, `argc`, which is reused as the line counter.

7. Passing a 2D array as a plain pointer

`main()` has a `char c[81][81]` array, but `A()` and `J()` declare it as
`char *c` and pass it on to `D()`, `L()` and `M()`, which declare it as
`char c[81][81]` again. The array survives the trip because only its address is
passed, but the changing types make it hard to see what `c` is.

8. Punch card codes hidden in parallel strings

The strings `K`, `d`, `m` and `r` are lookup tables. For the character at
position `k` of `K`, the characters `d[k]`, `m[k]` and `r[k]` name up to three
rows of the card that get a hole, using the row names in `B=" yz0123456789"`
(`y` is row 12 and `z` is row 11). `X()` skips the third hole for upper case
letters. Words such as `Brian Westley`, `winky` and `obfuscated` are mixed into
the tables so they look like messages rather than data. Characters missing from
`K` get a hole in every row.

9. Reused names

The name `t` is used for the order switch function, for a parameter of `Q(t,i)`
and for the temporary array `char t??( 81??)` in `G()`. Because the macro only
expands `t` when it is followed by `(`, these do not get changed. The name `r`
is also both a global table and a parameter (a row number in `R()`, `S()`,
`T()`, `Y()` and a string in `E()`, `F()`, `G()`).

10. A random comparison for scrambling

`F()` returns `rand()-rand()` from `H()` instead of a real comparison, so the
sorting loop in `A()` swaps lines at random. `H()` seeds the random generator
the first time it runs by writing the current time into a `static int` with
`time(&i)`, using `!i` as the "already seeded" check.

11. Reversing by printing backwards

`M()` prints the lines from the last to the first with `while(i--)`. Together
with the optional reversal in `J()` and the descending sort in `A()`, this makes
it hard to see which way the lines will finally come out.
