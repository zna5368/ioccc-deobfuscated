# 2000/jarijyrki

Difficulty: Hard

1. Single letter macros defined on the command line

The Makefile passes `-DU=40 -DT=98 -DQ=80 -DS=20 -DG=23 -Dp=7 -DC=XK_Up -D_=XK_BackSpace`
and more. The source only uses the letters, so `char E[U][U][T+1]` and `q l C ? Y--`
cannot be understood without reading the Makefile. `S` is used both as the cell
height and as the window margin, so the same letter means two different things.
Every name in the program is a single letter, including `_` for the backspace key.

2. Token pasting macros for Xlib calls

`#define b(b)XDraw##b(i,J,k,` glues its argument onto `XDraw`, so `b(String)` becomes
`XDrawString(i,J,k,` and `b(Arc)` becomes `XDrawArc(i,J,k,`. The macro also leaves
the parenthesis open, so the caller finishes the argument list. `t(White)` works
the same way and becomes `XSetForeground(i,k,WhitePixel(i,0));`.

3. Stringizing macro that builds an if/else chain

`#define i(i)o g>1&&*P==*#i){ g--; m[g-1]i##=m[g]; } else` turns its argument into a
string with `#i` and takes the first character with `*`, so `i(+)` compares the
formula character with `'+'` and then does `m[g-1] += m[g]`. Each use ends with
`else`, so `i(=)i(+)i(-)i(*)i(/)` chains five operator checks together. The name
`i` is also the name of the macro parameter and of the global `Display*i`.

4. The `=` operator that does nothing visible

`i(=)` expands to `m[g-1] == m[g]`, a comparison whose result is thrown away. The
only real effect is `g--`, which drops the top value from the stack.

5. Keyword and expression macros

`o` is `if(`, `a` is `for(`, `l` is `:q==`, `d` is `E[X][Y]` and `e` is `m[q]`.
`n` and `s` hide the screen position `(W-r)` and `(Z-u)` of the cell being drawn,
and `I` hides the entire long argument list of the pie chart arc calls.

6. Reused variable names

`P` and `X` are global ints (formula length, cursor column) but inside `v()` they
are local `char*` pointers into the formula. `K` is the recursion depth and is
then reused to hold the height of each histogram bar. `D` holds a row number, a
cell value, a cell formula or a column letter depending on when it is read.

7. Global variable used as a hidden loop counter

The global `q` holds the pressed key in `main()` but is the stack index inside
`v()`. After the loop `for(q=-1;++q<g;...)` finishes, `q` equals the stack size,
and the chart code after the `i:` label silently depends on that value.

8. Work hidden in for loop headers

Most loops have empty bodies with the work moved to the condition or increment.
`for(m[g++]=atof(D); P++&&isdigit(*P););` pushes a value and skips the digits of
a cell reference. The main loop draws the formula line in its increment
`b(String)S,S,d,P=strlen(d))`, and `for(;q==R;exit(write(1,E,z)));` is an `if`
that saves the sheet and quits on Escape. The row loop runs `sprintf` inside its
condition to prepare the row number.

9. Nested conditional operator instead of if/else

All key handling is one long `?:` expression: printable keys are appended to the
formula, arrow keys move the cursor, backspace removes a character. The value of
the expression is never used.

10. Using goto and side effects for charts

There is no separate chart code path. The characters `@` and `!` are treated as
syntax errors, which jump to the `i:` label. Just before the jump, the loop over
the stack has stored the sum (`F`) for `@` or the maximum (`A`) for `!`. The
error handler draws a pie chart or histogram when one of them is non zero and
then shows the formula text like any other error.

11. ASCII numbers instead of character constants

`64` is `'@'`, `33` is `'!'`, `65` is `'A'`, `32` is `' '`, and `q&96&&q<128`
tests for a printable character.

12. Tricky return values

`return !sprintf(D,...)` always returns 0 because `sprintf` returns a non zero
length, and `return !!strcpy(D,...)` always returns 1 because `strcpy` returns a
non null pointer. These are the success and failure results of `v()`.
`exit(write(1,E,z))` uses the number of bytes saved as the exit status.

13. Unused globals as zero constants

Globals start at zero, so `read(q,E,z)` reads from standard input and
`XCreateSimpleWindow(i,J,P,q,M,H,r,u,WhitePixel(i,X))` uses `P`, `q`, `r`, `u`
and `X` in place of the number `0` for the position, border and screen.

14. Multiplication as logical and

`isupper(*P)*isdigit(P[1])` multiplies two truth values, which is non zero only
when both are true.

15. Packed layout

The code is squeezed into a block of text with random spacing so statements and
macros run together across lines, hiding where each one begins and ends.
