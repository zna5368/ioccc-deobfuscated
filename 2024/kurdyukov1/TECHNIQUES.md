# 2024/kurdyukov1

Difficulty: Medium

1. Recursive `main` instead of loops

The program has no `for` or `while` loops. Every call to `main` prints at most
one character and then ends with `y>b?0:main()`, which calls `main` again until
the last row has been drawn. Each character on screen is one more level of
recursion. The rewrite uses an ordinary loop over rows and a loop over columns.

2. First-call initialization by testing a global

`!a ? a=2551443, ... : putchar(...)` uses the fact that global variables start at
zero. On the very first call `a` is 0, so the setup branch runs (start position,
moon phase). On every later call `a` is non-zero and the drawing branch runs. In
a normal program this setup just goes at the top of `main`.

3. Reusing one variable for unrelated values

`a` first holds the length of a lunar month in seconds (2551443), which is used
to compute the phase `z`. After that it is overwritten with `a=1-x` and holds the
width of the current row of the moon. Having one name mean two different things
makes the code hard to follow. The rewrite uses `SYNODIC_MONTH` and `width`.

4. Nested ternary operators and the comma operator

The whole program body is a single expression built from `?:` and `,`. For
example `++x>=a ? x=-b, y+=4, 10 : ...` means "if the row is done, go back to the
left edge, move down a row, and print a newline (character 10)". The comma
operator runs several expressions in order and keeps the value of the last one,
so state changes and the character to print get packed together. The rewrite
replaces these with `if` statements and separate `putchar` calls.

5. Multiple jobs for a single `putchar`

The argument of `putchar` picks between a newline (`10`), a space (`32`) and a
moon character, and it also updates `x`, `y` and `a` while doing so. Characters
are written as plain numbers (`10`, `32`) rather than `'\n'` and `' '`.

6. Hidden step size

`x` is moved forward twice for each margin character: once by `++x` and again by
`x=...x+1`. So the margin is checked only at every other column, which squashes
the circle sideways so it looks round in a terminal where characters are taller
than they are wide. Rows step by 4 (`y+=4`) for the same reason. The rewrite uses
`x += 2` and `y += 4` with a comment explaining why.

7. Indexing a string literal

`"#."[...]` picks a character straight from a string literal using a computed
index of 0 or 1, instead of an `if` that chooses between `'#'` and `'.'`.

8. Bit tricks and operator precedence instead of arithmetic

The phase is worked out with shifts and masks: `<<9` multiplies by 512,
`z>>8` gives 0 for the first half of the month and 1 for the second, and
`~z&255` gives `255 - (z % 256)`. The expression `x<a*(~z&255)>>8 ^ z>>8` also
relies on C's precedence rules (`>>` comes before `<`, which comes before `^`), so
you have to know those rules to see that it means
`(x < (a*(255-z%256))/256) XOR (waning half)`. The `^` flips which side is lit
when the moon goes from waxing to waning. The rewrite uses named variables and
brackets so the order is clear.

9. Unexplained magic numbers

`2551443` is the average lunar month (29.53 days) in seconds, and `592531` is the
time of a known new moon (7 January 1970) counted in seconds since the Unix epoch.
`44` is the radius of the drawn disc. None of them are named in the original. The
rewrite gives them `#define` names.

10. Implicit `int` declarations and shaped layout

`a,b=44,x,y,z;` and `main()` leave out the type, which old C treated as `int`.
The code is also laid out in the shape of a round moon, which hides how it is
built.
