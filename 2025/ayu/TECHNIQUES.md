# 2025/ayu

Difficulty: Terrifying

1. Single letter names that hide their meaning

The grid is `A`, the search queue is `B`, the attempt counter is `o`, the
player position is `r`/`c` and the platform functions are `a()` (print or
read a character), `b()` (sound) and `d()` (random seed).  The same letter is
used for many things: `R` is the reset function, the key parameter of `M()`,
a function-like macro and later a plain block-of-code macro; `f` is a global
variable and also a macro; `T` is a variable and a `goto` label in `L()`;
`c` is a global, a loop macro and a local loop counter that hides the global.

2. Macros that are not balanced

`#define R { h = 0; c(c, N) h += b[c]; if (h == N` and
`#define f ) { c(i, N) b[i] = ...` open a brace or close a parenthesis that
is finished somewhere else.  Code like `if (!r) R) f |= 1;` only makes sense
after the preprocessor has glued the pieces together.  The macros are
`#undef`'d and redefined in the middle of the function so the same name
means different code in different places.

3. Expression macro with side effects

`R(R, C)` is one long `&&` chain that checks that a neighbour cell is
inside the grid, is not a trap and is not visited, and then marks it,
pushes it on the queue and stops the search when row 0 is reached.  It is
used four times in one comma expression.  In simple.c it is the `visit()`
function.

4. "Custom operators" made of normal operators

The README's joke operators are just unusual spacing: `-~x` is `x + 1`,
`~-x` is `x - 1`, `c -=- !~c + c / N` is "subtract (-(c == -1) + c / N)",
which keeps the column inside the walls, `h -~- Z` is `h - Z + 1`,
`T ++> 2` is `T++ > 2`, `-T <o<< 5` is `-T < (o << 5)` and `N +~( r ...)`
is `N - r - 1`.

5. Reversed array indexing

`c[A]`, `r[A]`, `h[a]` and `0[B]["] "]` use the rule that `x[y]` is the
same as `y[x]`.  `0[R % 2 ? &r : &c] += ...` picks which variable to change
(row or column) with a pointer, then writes through it.

6. Lookup data hidden in a string

`H[] = "Aix6"` is a table: `'A'` and `'i'` are subtracted from the key to
get a direction number for arrow keys and vi keys, and the bits of `'x'`
and `'6'` (shifted by `6 - 2 * dir`) say whether to move the row or the
column and in which direction.  In simple.c this is a `switch` on the key.

7. Strings encoded as numbers

All text is packed 7 bits per character into 64-bit numbers and printed by
`w()`, lowest digit first.  For example `0x4bb66d820cbdb7cd + 34359738368 * o`
is `"Move <o>\033[K"` (the attempt digit is added at the 6th place) and
`17553960347 + (N << 21) - ...` is the "cursor up N + 2 lines" escape code.
The result messages (`"(O O)"`, `"\(^ ^)/"`, ...) are a table of such
numbers indexed by the result of `M()`.

8. Arithmetic instead of tables

The sound for each result is calculated by the polynomial
`(('3' - T * 5) * T - 154) * T + 144 + (...) << 8`, which happens to give
the right length and pitch for each result; `b()` then splits that number
again into a length (high byte) and a tone (`1309 / low part`).  Line
breaks of the board are found with `2977 % a(...)`, because `2977 % ' '`
and `2977 % ']'` are both 1.  The end screen keys `q`, Ctrl-C and Ctrl-D
are recognised by `2977 % T - ']'` and `T / -13` instead of comparisons.

9. One loop for the whole board

The board (entrance row, arena and exit row) is drawn by one `for` loop
over `i = -N .. N*N-1` whose increment expression prints every character
through nested `a()` calls.  The "is this the cursor" flag is stored in
`B[0]` (the search queue) as a scratch variable.

10. goto into the middle of an if/else

The label `T:` is placed inside the `else` of an `if` at the top of the
game loop, and both the end screen and the move handler jump back to it to
redraw.  Whether to redraw, show the header, quit or show the end screen is
encoded in the bits of the same counter `T`.

11. Bit tricks on global state

The attempt counter `o` doubles as a "game over" flag (bit 4) and is set
to 5 to force a new game.  The grid size `Z` borrows its lowest bit as a
temporary "rows are different" flag (`Z |= ...` then `Z &= -2`).  Visited
cells are marked by subtracting `2 * f` and restored by `A[i] &= 3`, which
only works because `f` is always a multiple of 2 at that point.  The cell
for the trap decision is given its value by `*a = 1 << (condition)`.

12. Hidden loop counts and assignments in conditions

`for n < 2 + o - *a` changes its own bound because the body writes `*a`, so
it runs exactly twice ("cell is safe" and "cell is a trap").  Conditions
also contain assignments such as `~(r0 = c)`, `t = !c[b]` and
`(*B = T && ...)`, and the start of `R()` sets four variables in one
comma expression: `o = !(r = c[A], f = r--)`.

13. Missing parentheses

The code relies on operator precedence everywhere, for example
`1[t + a] >> 6 - R - R` is `a[t + 1] >> (6 - 2 * R)` and
`(T > 1) + (T > 2) << 8` is `((T > 1) + (T > 2)) << 8`.  The Makefile
turns off `-Wparentheses` so the compiler does not complain.

14. Platform interface through three tiny functions

All terminal output and input go through `a()`, so the game also runs on an
Arduino.  `main()` uses the return value of `atexit()` (0) as the file
descriptor for `tcsetattr()`, and `E()` uses `u && fclose(u)` (also 0) the
same way.
