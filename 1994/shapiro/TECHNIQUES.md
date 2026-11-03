# 1994/shapiro

Difficulty: Medium

1. Code shaped into a picture

The whole source is laid out as a round clock face, with two "hands" made of
spaces. Tokens are split over many lines (`main(){int K` ... `,L` ... `,M`)
and padded with long comments so the code fits the outline, which hides how
short the real program is.

2. A second program hidden in the comments

Almost every comment is filled with capital letters such as
`MLSPXPMPVPOQN`. These are not noise: they are another C program (the clock
itself), written in a home-made hex code. Each letter from `J` to `Y` stands
for a hex digit (`J` = 0, `K` = 1, ... `Y` = 15), and two letters make one
byte, low half first. For example the first pair `ML` is 3 + 2*16 = 35 =
`'#'` and the next pair `SP` is 9 + 6*16 = 105 = `'i'`, the start of
`#include`. The program
only prints this hidden program, so reading the visible code alone tells you
very little.

3. The program reads its own source file

`J=fopen(__FILE__,"r");` opens `shapiro.c` itself. `__FILE__` is a
preprocessor name that turns into the name of the file being compiled, so the
program can find its own comments at run time without spelling out a file
name. simple.c names the file directly as `ENCODED_SOURCE`.

4. Code characters are also part of the data

The decoder skips every character outside `J`..`Y`, but it does not skip the
code. Capital letters in the real code, like the variables `K`, `L`, `M`, `J`,
the `L` in `FILE` and the `'J'` constants, are decoded too. The author had to
choose these names so that they fit into the hidden program at the right
places. Lower case letters such as `j`, `e`, `d` and `a`-`c` are put into the
comments only as filler, since they are ignored.

5. Single letter names for everything

`K` holds the low half of the byte, `L` is the character just read, `M` says
which half is expected next and `J` is the file. simple.c calls them
`low_nibble`, `c`, `have_low_nibble` and `source`.

6. The whole loop body in the `for` header

`for(;(L=getc(J))>=0;L>='J'&&L<='J'+15&&(...));` has an empty body. The
reading is done in the condition and all the work is done in the "increment"
part, which runs after each pass. simple.c uses a normal `while` loop with an
`if`/`else` inside.

7. `&&` and `?:` used instead of `if`

`L>='J'&&L<='J'+15&&(...)` only runs the last part if both tests are true,
because `&&` stops as soon as one side is false. Then
`(M=!M)?K=L:putchar(...)` flips `M` and uses the conditional operator to pick
between "save this letter" and "print the byte". Together they act like two
nested `if` statements.

8. Toggle written as an assignment inside a test

`M=!M` flips `M` between 0 and 1 and the new value is used straight away as
the condition. This makes the code alternate between the first and the second
letter of each pair, but it is easy to read it as a comparison.

9. Relying on operator precedence

`K-'J'|(L-'J'<<4)` needs the reader to know that `-` binds tighter than `<<`,
which binds tighter than `|`. Written out it is
`(K - 'J') | ((L - 'J') << 4)`, the low half plus the high half shifted into
place.

10. Hiding the range of letters

`L<='J'+15` hides that the last allowed letter is `Y`, so the reader has to
work out that the 16 letters are being used as hex digits.

11. Missing declarations and `exit` instead of `return`

`main()` has no return type and `exit` is used without including
`<stdlib.h>`, so the compiler has to guess their types. `exit(0)` is used
because `main` never returns a value, and `make` would stop if the program
exited with a random status.
