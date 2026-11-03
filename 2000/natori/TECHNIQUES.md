# 2000/natori

Difficulty: Medium

1. Double recursion through `main()` instead of loops

There are no `for`, `while` or `if` statements. `moon()` calls `main()`, and
`main()` just calls `moon()` again, so the two functions together act like a
loop. The same function is used for two different jobs: when its first
argument `_` is a row number (0 down to -22) it walks through the rows, and
when `_` is the magic value `-43` it walks through the columns of one row,
using `o` as the column counter (`++o` counts from -43 up to 0). The row
recursion goes deepest first and prints on the way back out, so the rows and
columns come out in the reverse of the order they were visited.

2. Short-circuit `&&`, `,` and `?:` used as control flow

`_--+22&&_+44&&main(_,-43,_)` only recurses while the row counter has not
reached -22 and stops the column calls (where `_` becomes -44). The comma
operator then evaluates `_&&(int)o`, and `?:` picks between "draw the rest of
the row and print a pixel" or "print a newline (10)". `l*l<4&&(...)` both
checks the point is inside the disc and protects `acos()` from an argument
outside -1..1.

3. One `putchar()` that prints everything

Every character, newline or pixel, comes from the single `putchar()` call at
the top of `moon()`. The newline for each row is printed when the column
counter reaches 0, which happens before the row's pixels because they are
printed while the recursion unwinds. This makes the output a leading blank
line followed by 23 rows, which simple.c writes as two plain loops.

4. Index into a string literal written backwards

`(condition)[" #"]` is the same as `" #"[condition]` because `a[b]` means
`*(a+b)`. The 0 or 1 result of the comparison chooses a space or a `#`.

5. Confusing variable names

The parameters are called `_`, `o` and `O`, which look alike and look like
the digit 0. `o` and `O` are even declared as `char**` in `main()` and cast to
`int`, so they appear to be pointers when they are really just numbers. The
first call passes `argc` as `_` (which must be 1) and the `argv`/`envp`
pointers as `o`/`O`, which only matter for being non-zero.

6. Unexplained magic numbers

`607728` is the time of a new moon in seconds since 1970, `2551443` is the
length of a lunar month in seconds and `405859.` is that month divided by
2*pi, so the result is the phase as an angle in radians. `4.7` and `1.57`
are approximations of 3*pi/2 and pi/2. `3-O*22-O*O` hides the circle formula
`124-(O+11)*(O+11)`, giving the half width of the Moon on each row.

7. Implicit declaration of `time()`

`<time.h>` is not included, so `time()` is assumed to return `int`. This saves
a header and keeps the arithmetic in plain integers.
