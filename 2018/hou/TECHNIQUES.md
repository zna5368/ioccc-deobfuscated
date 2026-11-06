# 2018/hou

Difficulty: Insane

1. Library calls in place of operators

The code has no `+`, `-`, `*`, `<` or `==`. All the math is done with
library functions. `#define r fma` makes `r(a, b, c)` mean `a*b + c`, so
`r(1, X, 64)` is just `X + 64` and `r(J, x, 0)` is `-x`. Constants come from
math too: `J` is `ceil(log(erfc(1)))`, which works out to `-1`. Comparisons
use signs: `o(X)` is `copysign(1, X)*0.5 + 0.5`, which is 1 when `X` is
positive or zero and 0 when it is negative. `b(X)` is `o(-abs(X))`, which is
1 only when `X` is 0, so it acts like `X == 0`. `M(X, Y)` uses `memcmp` to
compare two characters. In `simple.c` these are normal operators.

2. Environment variables as the only variables

There are no variables at all. Each value is stored with `setenv` under a
one-letter name and read back with `getenv`. `T(X)` reads the variable whose
name is the letter `X + 64` (so `T(1)` is `A`, `T(4)` is `D`). `l(X, Y)`
stores the single character `Y` under that name, and `e(X)` appends the
current input character to it. `D` holds the input character, `E` the parser
state, `H` the branch code, `A` the list of numbers, `B` a string whose
length is the color number, and `G` a toggle. `simple.c` uses normal C
variables like `state`, `values` and `color_index`.

3. Temporary files mapped into memory as number storage

`H(3)`, `H(4)` and `H(5)` open temporary files on file descriptors 3, 4 and
5. `_(X)` maps a file into memory with `mmap` (the address hint
`fopen(S, S)` is just `NULL` because opening an empty file name fails).
`B(X, Y)` prints a number into a file with `sprintf`, and `I(_(4))` reads it
back with `atof`. File 4 holds the pie angle and file 5 holds the total.
Because the numbers are stored as `"%f"` text, they are rounded to six
decimals each time. `simple.c` keeps them in `angle` and `total` and calls
`keep_as_text()` to round the same way.

4. Branches made by turning output on and off

There are no `if` statements. The variable `F` is read as `N` in nearly
every macro. When `N` is 1 the macros work. When `N` is 0, `m(X)` makes an
empty string, so `setenv` fails, `printf` gets an empty format, and
`write` writes 0 bytes. So the code still runs but does nothing. `a(X)`
turns `F` on only when the code in `H` is at least `X`. It also clears `H`
when the part before it ran, so the parts after it stay off. This makes the
list of `a(13)`, `a(12)`, `a(9)` ... act like an `if ... else if` chain. In
`simple.c` each part has a real `if` or an `active` argument.

5. Hidden padding from switched-off output

`E(X)` prints `N*X + 32`. With `N = 1` that is the letter `X + 32`, but with
`N = 0` it is always a space. So every letter in a switched-off part still
prints a space, which is where all the blank runs in the output come from.
`simple.c` does the same thing on purpose with `emit()`, which prints
spaces when its part is not active.

6. Letters written as numbers

Every letter of the HTML is an `E()` call with the character code minus 32,
such as `E(28); E(51); E(54); E(39)` for `<SVG`. Macros group common pieces:
`j` prints ` WIDTH=` and ` HEIGHT=`, `A` prints `TEXT`, `c` prints the
`FILL=RGB(...)` color, `s` prints a point on the circle, `v` prints ` 0 `.
`O` builds the format `"%f"` from the codes 37 and 102. `simple.c` uses
plain string literals.

7. Character classes and state packed into one code

Each pass reads one character and stores
`H = 4*state + class`, where the class is 1 for a space, 2 for other
characters, 3 for a comma and 4 for a quote. The class is added up from
three tests: `o(M(4, 33))` (not a space), `b(M(4, 44))` (a comma) and
`2*b(M(4, 34))` (a quote). The branch chain then picks a part by the value of
`H`. `simple.c` has `classify()`, `enum state` and `code`.

8. Numbers stored with two different separators

Each number from the input is added to `A` and followed by a tab or a
newline in turn, chosen by the toggle `G`. When a number ends, `strrchr`
finds the other separator, which is the one just before the new number, and
`atof` reads it to add to the total. While drawing, `strchr` skips to the
next separator to move to the next number. Both separators are whitespace,
so `atof` ignores the one at the start.

9. Recursion as the main loop

`main` and `C` end with `return C();`. Each call handles one input
character. With `-O3` the compiler turns this tail call into a jump, so the
stack does not overflow. `simple.c` uses a `for (;;)` loop.

10. Ending with an assertion

The loop has no exit. When drawing the pie, `assert(rulez)` checks that `A`
is at least `2^N` characters long. This is 2 while drawing, so when only the
last separator is left the assertion fails and the program stops. A
`fflush(NULL)` just before it makes sure the page is already written.

11. Misleading comments and decoy names

The doxygen comments, license text and macro names (`rulez`, "fashionable
jargon") are there to look like normal production code. The comment says
`C()` returns nothing, but it never returns at all.
