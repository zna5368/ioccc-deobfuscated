# 1992/buzzard.1

Difficulty: hard

1. A made-up assembly language built from macros

The program body is written in a tiny assembly language whose instructions are
one-letter macros: `Q(b,a)` stores, `A`/`S`/`M`/`D` add, subtract, multiply and
divide, `Z(a,d)` and `E(a,d)` are conditional jumps, `J(x)` is a jump, `l` is a
label and `H` halts. The code then looks like `n(B)Z(a,4)n(P)E(a,1)` instead of
C. `simple.c` turns every instruction into a named function such as
`jump_if_zero` and `set_positive` and calls them one per line.

2. Macros that only rename other macros

`b`, `c`, `e`, `f`, `g` and `h` call each other in a circle, for example
`f(x)` is `x(e)` and `e(x)` is `y`, so `f(g)` becomes `g(e)`, then `e(b)`, then
`y`. Each of these chains just ends in the variable `x` or `y`. Other helpers
shorten common patterns: `j(x)` is `y = x`, `k(x)` is `a = y - x`, `m(B)` is
`B(a,a)`, `n(P)` is `P(a,y)`, `o(x,y)` is `x *= y`, `d(x)` is `x -= 1` and
`i(c,a,b)` swaps the order of its arguments to call `a(b,b,c)`. The author says
they are mostly there to make you run the file through the preprocessor, which
produces a huge unreadable list of assignments. `simple.c` uses the real
variable names directly.

3. Every statement runs on every pass (no if or goto)

The `for(;!w;)` loop runs the whole program from top to bottom again and again.
There are no `if`, `goto` or comparison operators. Instead the variable `r` is 1
when the code should really run and 0 when it is being skipped, and every store
goes through `_(a)`, which is `a += (new - a) * r`. When `r` is 0 nothing
changes. This is `store()` in `simple.c`, and `r` is called `executing`.

4. Labels and jumps made from arithmetic

`p` holds the label to run next. A jump only sets `p` and turns `r` off. Each
label `l` checks if `p` equals its own number and turns `r` back on if so. The
label numbers are not written anywhere: `q` is set to 0 at the top of the loop
and counted up by each `l`, so the eleventh `l` is label 10. `q` is also used as
a hidden constant, for example `E(a,q)` jumps to the very next label (3) and
`D(a,x,q)` divides by 10. `simple.c` gives the labels names in an `enum` and
writes these numbers as constants.

5. Comparisons using only division

`R` adds 2047, divides by 2048, adds 8191 and divides by 8192. Because integer
division rounds toward zero, this turns any positive number into 1 and zero or a
negative number into 0, as long as the number is not too big. `P` uses this for
"greater than 0", `B` adds the results for `a` and `-a` to test "not zero", and
`L` squares `p - q` to test "equal". Conditional jumps such as
`Z(a,d)` compute `p = (p - d) * a + d`, which is `p` when `a` is 1 and `d` when
`a` is 0. These are `positive()`, `set_nonzero()`, `label()` and
`jump_if_zero()` in `simple.c`.

6. Safe division by a variable

Because skipped code is still calculated, `y / x` would crash if `x` is 0 even
in code that is not "really" running. `U(c,a,b)` first changes the divisor to
`(b - 1) * r + 1`, which is 1 when skipping, then divides and puts `b` back.
This is `safe_divide()` in `simple.c`.

7. Printing strings with a number format

The only output is `printf("%x\n",y)`. The messages are numbers whose hex
spelling is a word: `2989` is `0xbad` and `2766` is `0xace`. To print normal
answers in decimal, the code at label 8 rebuilds the result one decimal digit
per hex digit (binary coded decimal), multiplying the place value by 16 each
time, so `%x` shows the decimal digits. `simple.c` writes the words as `0xbad`
and `0xace`.

8. Arguments read inside the loop

`G(x,y)` calls `atoi(argv[y])` on every pass of the loop, and the first one
uses the label counter `q` (which is 1 at that point) as the index. `simple.c`
writes `argv[1]` and `argv[2]`.
