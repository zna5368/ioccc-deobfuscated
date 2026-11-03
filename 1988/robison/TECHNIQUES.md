# 1988/robison

Difficulty: Hard

1. Macro disguised as a keyword

`#define whi1e(j,k) printf(j,k); fflush(stdout)` uses a digit `1` in place
of the letter `l`, so `whi1e("%s","2.")` looks like a loop but is really a
call to `printf`. The source is laid out so that every line starts with a
`while` or `whi1e`, which hides the fact that some lines only print.

2. Macro hiding an array access

`#define o E[w]` makes `o` look like an ordinary variable. Every `--o` is
actually `--E[w]`, an element of an array indexed by `w`, and because `w`
changes inside the loops, `o` refers to different memory at different times.

3. Single-letter names spelling a word

The globals `w,h,i,l,e` spell `while`, and the extra ones (`x`, `y`, `z`,
`E`) say nothing about their purpose. `z` is the digit count, `y` the base,
`e` the number of terms, `E` the fraction digits, `i`/`l`/`h` the quotient,
running value and loop counter, and `x` a scratch variable.

4. Doing everything with `while` and `--`

The program avoids `=`, `+`, `-`, `/`, `%` and `for`. Everything is built
from small loops that rely on `unsigned char` wrapping around (0 minus 1 is
255):

- `while(--a);` sets `a` to 0.
- `while(--a) --x; while(--x) --b,--a;` adds `a` to `b`. The first loop
  moves `a` into `x` as a negative number, the second moves it back into
  both `a` and `b`. Example: `while(--y) --x; while(--x) --e,--y;` is
  `e += y`.
- `while(--x) --h;` with `x` at 0 runs 255 times, which is the same as
  `h += 1`.
- `while(--w) { while(--x) --o; }` sets every `E[1..255]` to 1.
- `while(l>=w){ --i; ... }` subtracts `w` from `l` one step at a time while
  counting down `i`, which is division with `i` holding the negative
  quotient and `l` left as the remainder.

5. Values stored negated

Because only `--` is allowed, the quotient in `i` and the carry in `l` are
kept as negative numbers and turned positive by counting down from zero
(`while(--i) --l; --l;` makes `l = -i`). The printed digit is produced the
same way with `while(--i) --h;` followed by `--h`.

6. Loops that restore their own counter

Inner loops such as `while(--w) --l,--x; while(--x) --w;` count `w` down to
0 and then rebuild it from `x`, so the outer `while(--w)` loop still sees the
right value. It looks like `w` is destroyed, but it is put back.

7. Misleading layout

Every line is split into three columns (`while`, `(condition)`, body), which
makes the loops look uniform and hides where blocks open and close. Braces
such as `{--i;` and `--w;--l;}}` are buried at the start and end of lines.

8. Unusual argument parsing

`while(c>=2){ z=atoi(v[1]),--c; while(c>=2) y=atoi(v[2]),--c; }` uses loops
with a decreasing `argc` instead of `if` statements to read the optional
arguments.

9. K&R function definition

`main(c,v) char *v[];` uses the old style declaration with an implicit `int`
return type and an undeclared `atoi`.
