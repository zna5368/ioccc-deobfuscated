# 2018/algmyr

Difficulty: Hard

1. Base64-style packed font data

The big string `G` at the top holds the font. Each character after `' '` is a
6-bit digit (`' '` is 0 and `'_'` is 63). Seven digits make one 42-bit
number, and each byte of that number is one 8-pixel column of a glyph. Most
pixels are off, so most digits are `' '`. This makes the string look like
blank space, and the size tool skips whitespace even inside strings.
`simple.c` replaces the string with a plain table, `font[95][5]`, plus
`newline_glyph`.

2. Hidden range check and table indexing

`F()` picks a glyph with `(o*(136-o)>=1260)*7*(o-9)+G`. The test
`o*(136-o) >= 1260` is a quadratic that is true only when `10 <= o <= 126`,
so it acts as a range check without any `if`. The digit loop
`(o+=3)>>2<5-t` with the shift `2*o%8` reads just enough digits to reach
byte `t` of the 42-bit number. In `simple.c` this is `glyph_column()`.

3. Unused and overwritten parameters

`F(int o,int n,int t)` sets `n` and `o` again before using them, so the
values passed in do nothing. A call like `F(*i++, r<5, r)` passes `r<5`,
which looks important but is thrown away.

4. Recursive sine table built as a side effect

`T()` fills the sine table `L` with a recursive rotation formula
(`sin(k+1) = sin(step)cos(k) + cos(step)sin(k)`). The step constants
`6.135885e-3` and `1-1.882472e-5` are just `sin(2*pi/1024)` and
`cos(2*pi/1024)`. `T` is called inside `f()` on every lookup, but it only
does work the first time (it stops when `L[0x3ff]` is already set). Its
return value then flips the period: `s<<!!T(...)` is `s` on the first call
and `2*s` after that. The only first call is `f(0)`, which gives 0 either
way. `simple.c` builds the table once in `build_sine_table()` and always
uses the period `2*SAMPLE_RATE`.

5. Out-of-bounds read hidden by memory layout

Interpolating in `f()` can read `L[0x400]`, one past the end of the array.
In the original binary that memory holds the global `int l` (16), which
read as a float is almost exactly 0. `simple.c` makes the table one entry
longer and puts the correct value, `sin(2*pi) = 0`, there.

6. Magic numbers in single-letter globals

`s,p,e,c,t,a,l` are set in `main` to 44100, 25, 55, 256, 75, 7 and 16 (sample
rate, divisor, frequency unit, pattern count, columns per second, row
spacing, base frequency). The comment `/* l e a k a g e */` spells out the
names. Some of them are reused as temporary variables (`r`, `a`, `t`, `c` in
`d()`). `main` also declares local `l` and `t` that hide the globals with
very different meanings (`t` becomes a float pointer). `simple.c` replaces
them with named `#define` constants and local variables.

7. Disguised math formulas

The chirp `f(s/2-e*a*y+e*a*y*y*(t/p)/(s/p))` adds a quarter period
(`s/2`) to turn sine into cosine, and `(t/p)/(s/p)` is just `75/44100`
written in a less obvious way. The window `z*(1+3*z+2*z*z)/6` with
`z = sin^2` looks like a polynomial. It is really a 4-term cosine-sum
window (like Blackman/Nuttall) rewritten in powers of `sin^2`.

8. Short circuit operators as control flow

`T()` is one `return` built from `||`, which replaces both the stop test and
the recursion. In `C()`, `c+=(1&P>>x&&P+(*h+=...))` uses `&&` so the sample
is added only when pixel `x` is lit, and in the same step it counts the lit
pixels. `int x=*h&&1>ungetc(*H++,*f)` only pushes back a usage character
when help mode is on.

9. Loops with all their work in the header

Many `for` loops have empty bodies and do everything in the condition and
increment, e.g. the pixel loop in `C()`, the digit loop in `F()`, the free
loop `for(; --c; E P[c]))` and `while(fclose(f[--w]),w);`. The
comma operator packs several statements into one expression.

10. Packed command-line flag comparison

`r=v[1][0]*c+v[1][1]` joins the first two characters of the argument into
one number (`c` is 256). `11620` is `"-d"` and `11624` is `"-h"`.

11. String offset trick for optional text

`sprintf(H,"\0" "Usage\n" ... +(q-1&&r==11624),v[0])` begins the format with
`"\0"`. Adding 0 gives an empty string, and adding 1 skips the `\0` and
gives the usage text. So the help message is chosen without an `if`.

12. Printing a string through `ungetc`

The usage text is not printed directly. Each character is pushed back into
`stdin` with `ungetc` and then read again by the normal `getc` input loop,
so it is encoded like any other input. Pushing the final `'\0'` makes
`ungetc` return 0, which sets the "finished" counter and ends the loop.

13. Self-describing return values

`C()` returns `h-i-r+1`, which is zero only for the last channel. So
`while(C(b++,t,w,...));` steps through the interleaved channels without a
counter. Also, `if(R-Z*w)break;` hides an `fread` inside the `R` macro.

14. Macros that hide calls

`M`, `R` and `E` expand to `malloc(sizeof(`, an `fread` of one column, and
`free(`. Their parentheses are unbalanced, so `M float)*Z)` does not look like
a function call.

15. Shaped layout

The code is drawn as a large triangle with misleading indentation. This
hides which statements belong to which loop.
