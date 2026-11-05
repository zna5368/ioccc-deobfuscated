# 2013/endoh3

Difficulty: Medium

1. Two loops squeezed into one `for`

`for(;d>2e3*c?...:1;a=2)++d<24?...:putchar(...);` is the only statement in the
program. Normally there would be a loop over the notes with two smaller loops
inside it. Here they share one loop, and the value of `d` decides which job the
current pass does, so the reader has to work out the hidden structure.

2. Work hidden in the loop condition

A `for` condition usually only tests something. This one reads the next note and
does all of the setup for it: `c=1,scanf(" %c%f",&a,&c),d=55-a%32*9/5,...`. The
loop only continues if the last value in that list is not zero, so reading input
and deciding when to stop are the same expression.

3. Ternary operator instead of `if`

`d>2e3*c?...:1` and `++d<24?b*=89/84.:putchar(a=b*d)` are `if`/`else` decisions
written as `condition ? yes : no`. In the first one, the `:1` means "keep going
without reading". Packing the decisions this way removes the braces and
indentation that would show the shape of the program.

4. Comma operator

`c=1,scanf(...),d=...,b=d>9,d=d%13-a/32*12` is five statements joined by commas
so that they fit where only one expression is allowed. They run from left to
right, and the value of the whole list is the value of the last part.

5. One variable with several jobs

`d` starts as the argument count, then holds a code for the letter, then a
counter for raising the pitch, and finally the time position in the wave. `a`
holds the note letter, then an output byte, then a marker value. A single name
therefore means something different on each line.

6. Argument count used as a starting value

`main(d,e)` looks like it takes arguments, but the program never treats them as
arguments. `d` is the argument count, which is always at least 1, and `e` is
never used. The only purpose of `d` being positive is to make `d>2e3*c` true on
the first pass, since `c` starts at 0, which triggers the first read.

7. Magic arithmetic instead of a table

`55-a%32*9/5`, followed by `d%13` and `a/32*12`, turns a letter into a musical
position with no table and no explanation. `a%32` makes upper and lower case
letters count alike, `*9/5` stretches the letters out to uneven spacing, `%13`
wraps the result around, and `a/32` is 2 for uppercase and 3 for lowercase. The
numbers were found by brute force, so they cannot be understood just by reading
them.

8. Repeated multiplication instead of a power function

`b*=89/84.` runs once per pass rather than calling `pow()`. The fraction 89/84
is a close approximation of a number that has no exact fraction, and the number
of passes is decided by how far `d` starts below 24. The exponent never appears
anywhere in the source, because it only exists as a loop count.

9. Comparison result used as a number

`b=d>9` stores the true or false result of a comparison, which is 1 or 0, in a
`float`. It looks like a test, but it is really an assignment that sets a
starting value of 0 for rests, so everything multiplied by `b` afterwards
stays 0.

10. Implicit conversion doing the real work

`putchar(a=b*d)` assigns a `float` to a `char`. There is no cast or `%` to
suggest it, but the conversion throws away all but the lowest 8 bits of the
whole number part, so the steadily growing value `b*d` wraps around at 256. A
key part of the program happens in a conversion that is not visible in the
source.

11. Marker value instead of checking for end of input

`scanf` returns a value that says whether it succeeded, and the program ignores
it. Instead, the loop's third part sets `a=2` after every pass. If `scanf` finds
no more input it leaves `a` alone, and the arithmetic on 2 happens to produce 0,
which ends the loop. The `a=2` looks meaningless until the arithmetic is worked
through by hand.

12. Default value set before an optional read

`c=1,scanf(" %c%f",&a,&c)` reads a letter and then an optional number. When no
number follows, `scanf` leaves `c` unchanged, so the 1 assigned just before acts
as the default. Nothing in the code says that the number is optional.

13. Missing types and headers

`char a;float b,c;main(d,e)char**e;{` relies on old C rules. `main` and `d` have
no written type so they default to `int`, the parameter type is given in the old
style between the `)` and the `{`, and there is no `#include` for `scanf` or
`putchar`. The global `a`, `b` and `c` also start at 0 without saying so, which
the first pass of the loop depends on.

14. Number written in scientific notation

`2e3` is simply 2000, the length of a note. Writing it this way saves a character
and makes it harder to recognise as a count of samples. It also makes the
comparison `d>2e3*c` use floating point numbers even though `d` is an `int`.
