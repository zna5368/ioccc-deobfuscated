# 2013/dlowe

Difficulty: Easy

1. Meaningless variable names

`main(a,b)` hides `argc` and `argv`, and `c`, `d` and `e` are single letters
that say nothing about their job. `c` is the position of the smallest number in
`argv`, `d` is the position of the largest number and `e` is a loop counter.

2. Macro with a name that says nothing

`-D'_(x)=strtof(b[x],0)'` is given in the Makefile, so the source file only shows
`_(e)` and `_(c)`. A reader of `dlowe.c` cannot see that `_` converts an argument
from text to a number. The macro also uses `b` without taking it as a parameter,
so it only works where the variable `b` exists.

3. Headers supplied by the compiler command

`dlowe.c` has no `#include` lines. The Makefile passes `-include stdlib.h
-include stdio.h` so `strtof` and `printf` are declared without the source file
showing where they come from.

4. Old style function definition

`main(a,b)char**b;{` is a K&R definition. The return type is left out so it
defaults to `int`, `a` has no type so it is also an `int`, and the type of `b`
is given after the parameter list. `main` has no `return` either, which is
allowed because reaching the end of `main` returns 0.

5. Chained conditional operator used as an if statement

`_(e)<_(c)?c=e:_(e)>_(d)?d=e:7;` is an `if`/`else if` written as one expression.
The `?:` operator always needs a final branch, so the magic number `7` fills
that place. The value is thrown away, and any other number would work.

6. Reused variable

The first loop counts `e` down until it reaches 0, and `e` is never reset. The
`while(++e<a)` loop relies on that leftover 0 so that it starts at 1, which is
the first number on the command line.

7. Hand-rolled UTF-8

`"\xe2\x96%c"` with `129+...` builds each bar one byte at a time. The bytes
`e2 96 81` to `e2 96 89` are the UTF-8 encoding of the block characters U+2581
to U+2589, so the number added to 129 selects the bar. The source never shows
that any block characters are involved.

8. Unrelated variable used to convert a type

`**b=8*(...)/(...)` stores the result in the first letter of the program name,
which is `argv[0][0]`. The name is never used again, so this does no harm. The
point is that `**b` is a `char`, so the assignment cuts the floating point
result down to a whole number that can be passed to `%c`. Without it `printf`
would be given a `double` and print garbage.

9. Hidden reliance on floating point division

`8*(_(e)-_(c))/(_(d)-_(c))` divides by zero when all the numbers are equal, or
when there is only one number. Dividing whole numbers by zero would crash, but
these are floating point values, so 0 / 0 gives the special value NaN (not a
number) instead. Converting NaN to a `char` is undefined behaviour in C, and it
happens to give 0 on x86, so the lowest bar is printed. The code gives no sign
that any of this is going on.

10. Dense formatting

The whole program is packed into two lines with no spaces or indentation, and
the number parsing is repeated inside the expressions rather than given a name.
This makes it hard to see where each statement starts and ends.
