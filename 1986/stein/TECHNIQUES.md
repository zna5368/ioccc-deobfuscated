# 1986/stein

Difficulty: Medium

1. Recursive `main` used as a loop

`main` calls itself. With a positive `b` (the real `argc`) it starts the
program, and `main(~5, ...)` (`~5` is -6) starts a chain of calls
`main(b+1, Z+1)` that stops when `b` reaches 0. Each call handles one digit of
the clock, so the recursion is a hidden loop that runs six times. simple.c uses
a `for` loop.

2. `&&` and `||` instead of `if`

The whole body is one expression `b=(b>=0||(...))&&(b<1||(...))`. Because `||`
skips its right side when the left side is true, `b>=0||(X)` means "if `b` is
negative, do X", and `b<1||(Y)` means "if `b` is at least 1, do Y". The final
value stored in `b` is never used. simple.c uses normal statements.

3. Comma operator to chain statements

Inside the parentheses, steps such as `main(b+1,Z+1),*Z=...,O/=o,_/=25` are
joined with commas so that several statements fit where only one expression is
allowed. They run left to right.

4. Many radixes packed into one number

`_=33303285` looks like a random number, but written in base 25 its digits are
3, 10, 6, 10, 6, 10. Each call takes `o=_%25` (the lowest base 25 digit) and
then `_/=25`. These are the sizes of each place of a clock: seconds and minutes
use 10 and 6, hours use 10 and 3. So `O%o` gives one clock digit and `O/=o`
moves to the next. simple.c names this `PACKED_RADICES`.

5. Octal constant

`0250600` starts with a 0, so C reads it as octal. In decimal it is 86400, the
number of seconds in a day. `time(&b)%0250600` is the number of seconds since
midnight (GMT).

6. Meaningless one letter names

The globals `O`, `o` and `_` hold the seconds, the current radix and the packed
radixes. They are declared without a type (`O;o;`) so they are implicit `int`.
The parameters `b` and `Z` are really `argc` and `argv`.

7. Reusing variables for other jobs

`b` is `argc`, then it receives the time from `time(&b)`, then it becomes the
counter for the recursion, and finally it stores the result of the whole
expression. `Z` is `argv` at the top level but inside the recursion it points
into the bytes of the program name.

8. Casting `argv` to the wrong type

`typedef char**z` makes `z` mean `char **`, so `*(z*)Z` reads `argv[0]` (the
program name string) but treats it as an array of `char *`. The digits are
written into that memory with `*Z=...+'0'`, and `write(1,*(z*)Z,9)` prints 9
bytes from it. The program name is used as a free buffer. The original source
in stein.orig.c used `typedef char*z`, which put one digit per byte so the
output was the time `HHMMSS`. With `char **` each digit takes a whole pointer
sized slot, so on a 64 bit machine the 9 bytes printed are the first digit,
seven zero bytes and the second digit. simple.c keeps this behavior.

9. Implicit declarations

`time` and `write` are used without any header, so the compiler guesses that
they return `int`. The arguments of `main` use the old K&R style with
`b` getting the default type `int`.
