# 2013/morgan1

Difficulty: Hard

1. Numbers packed into a string

The long string `n` holds the floating point numbers, five characters each. In a
group like `VS6>7`, the first character holds the sign and the exponent and the
other four are base 64 digits of the mantissa. The nested loops
`for (r = 0, c = ...; --y; r = (r - 47 + *c--) / 64);` and
`exp2((*c - 0x2f >> 1) - 21) * (2 * (*c & 1) - 1)` rebuild each number when it
is needed. Nobody can read a value such as 0.387 from the source because it is
stored in a format that the author invented.

2. Every character shifted by one

`for (...; (*c++)--; );` subtracts 1 from every character before the string is
used. All the text in the source is therefore wrong by one letter, so `Nf` is
really `Me` and `&15e0` is really `%04d/`. This also explains the `\1` bytes:
they turn into the `\0` that ends a C string, but the loop does not stop at them
because it tests each value before changing it.

3. One array used for everything

The same array `n` holds the labels, the numbers, both format strings and the
output buffer. `scanf(n + 392, ...)` and `sprintf(n + 416, n + 377, ...)` hide
their format strings behind plain offsets. The record for the Sun even overlaps
the format strings and the buffer, so the text is decoded as numbers as well.
That does no harm because the first number in the record is 0, which cancels the
others out.

4. Pointer arithmetic to find text

`XDrawString(D, W, G, d + 7, ++y, c - ' ', 2)` draws the two letter label. `c`
was left near the end of the record by the decoding loop, and `' '` is really
the number 32, the distance back to the start of the record. The label is only
found because of where an earlier loop happened to stop.

5. One letter variables that change meaning

`y`, `m` and `d` start as the year, month and day but are later used as a loop
counter, a planet number, a key symbol and screen coordinates. `a` is a century
count in one place and an eccentricity in another, and `l` is a day count and
then an angle. Knowing what a variable means on one line says nothing about the
next line.

6. All the work done in `for` headers

The whole program is `main()`, and the body of the main loop is one inner `for`. The
date conversion and the code that draws the date are in the condition of the
`for`, while the code that reads a key is in its increment. In the inner `for`,
the drawing happens in the increment too, so the code that runs last in each
pass is written first.

7. Comma operator

`XGetWindowAttributes(D, W, &A) && XClearWindow(D, W), b = floor(...), m = ...,
d = ..., y = ...` is a chain of statements joined by commas so that they fit
where only one expression is allowed. They run from left to right, and only the
last one counts as the value of the chain.

8. Ternary operator instead of `if`

`j > m++ ? y--, m += 12 : j--;` is an `if`/`else` written as an expression. The
`j--` on the right does nothing useful and is only there because a ternary needs
two sides. `13 < m-- ? m -= 12 : y--` works the same way.

9. Logical operators instead of `if`

`&&` and `||` skip their right side when the left side already decides the
result. `XNextEvent(D, &V) || (j += ...)` and
`!(m == 9 && ...) && XDrawArc(...) && XDrawString(...)` use that to run code
conditionally, with no `if` in sight.

10. Function results used as loop conditions

`!XDrawString(...)` is the condition of the main `for`. `XDrawString` always
returns 0, so the condition is always true and the loop never ends. It looks like
a real test, but it is an infinite loop in disguise.

11. Bit mask instead of a condition

`j += -(V.type == KeyPress) & (...)` turns the comparison into 0 or -1. In binary
-1 is all ones, so the `&` keeps the number of days on a key press and clears it
to 0 for any other event. This is an `if` done with arithmetic.

12. Numbers written in misleading forms

Hexadecimal, decimal and character constants are mixed for no reason: `0x62e` is
1582, `0x76c` is 1900, `'d'` is 100 and `0x1a0` is 416. `63 * 365 + 45` looks
like a number of days but is 23040, a full circle in the 1/64 degree units of
X11. Writing the year 1582 two ways on one line, `1582 > y || y == 0x62e`, hides
that both sides are the same value.

13. Inconsistent comparison order

Comparisons switch sides from one to the next, such as `1582 > y`, `11 > m`,
`d < 5` and `150 < fabs(...)` next to `fabs(...) > 0x96`. The reader has to stop
and flip each one to compare them.

14. Unusual ways of writing common operations

`d++[o]` is `o[d++]`, because `a[b]` and `b[a]` mean the same thing in C. `*o` is
`o[0]`. `m = !m` is a way to write `m = 0` because `m` is never 0 at that point.
`m++ ^ 12` uses exclusive or as a "not equal" test, and `!!r` turns any non-zero
number into 1.

15. Values left over from earlier code

`for (l = ...; 1e-6 < fabs(r); ...)` tests `r` before the loop has set it. It
passes only because `r` still holds a digit total from the decoding loop. The
same trick appears in `-152385 < j ? ... : d`, where `d` is left over from the
last key press, so it is 0, or from the typed date on the first pass.

16. Assignments buried inside other expressions

`DefaultScreen((D = XOpenDisplay(0)))`, `fmin(d = A.width / 2, y = A.height / 2)`
and `sin(l += o[5])` change variables in the middle of a bigger calculation. The
changes are easy to miss because they look like plain arguments.

17. Formulas without any names

The calendar conversions and the orbit equations are written out inline, with
bare constants such as `30.6001`, `730550` and `.802601` and with most of the
parentheses removed. Nothing says what a line calculates, so the reader has to
recognise the formula itself to work out what it is for.
