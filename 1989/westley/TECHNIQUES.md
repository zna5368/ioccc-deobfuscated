# 1989/westley

Difficulty: Hard

1. Source code that reads forwards, backwards and in ROT13

The file is written so that it is still a C program if every line is reversed
and/or every letter is ROT13'd. Most lines contain forward code and backward
code side by side, and comments switch between them: `/**/forward/*/drawkcab/**/`
compiles the forward part and hides the backward part inside a comment, and
when the line is reversed the opposite part ends up inside the comment. Some
pieces are palindromes such as `tang ,gnat` and `tang^tang`, so they work in
both directions and are shared. To read the real program you have to run it
through the preprocessor, which removes all the comments. `simple.c` contains
only the forward code.

2. Variable names that are ROT13 pairs

Names were chosen so they still make sense after ROT13: `main` becomes `znva`,
`pain` becomes `cnva`, `irk` becomes `vex`, `tang` and `gnat` are both ROT13 and
backwards spellings of each other, and `NOON`/`ABBA` are palindromes. None of
them describe what the variable holds. `tang` is the argument count, `gnat` is
the character just read, `ABBA` is a flag and `noon` and `raeN` are the
character and flag passed to the printing function. `simple.c` uses `mode` and
`c`.

3. Command line flags hidden in the argument count

Only the number of arguments matters, not what they are. After `--tang`, bit 0
of the count (`--tang & 1`) turns on ROT13 and bit 1 (`gnat & 2` with
`gnat = ~tang`, and later `tang & 2`) turns on reversing. `simple.c` names
these bits `MODE_ROT13` and `MODE_REVERSE`.

4. Detecting which version is running

`ABBA = 'N' == 65` is false in the original because `'N'` is 78. In the ROT13
version the same line reads `'A' == 65`, which is true. The flag decides
whether to call `getchar` or `trgpune` (which the Makefile defines as
`putchar`), so each version still reads input with the right function even
though ROT13 swaps the two names. `simple.c` only keeps the path the original
version takes and calls `getchar()` directly.

5. Recursion instead of loops and buffers

There are no loops. `main` reads one character and calls itself
(`main(tang+1, gnat, 1+gnat)`) to read the rest, stopping when the character
is `EOF` (`(int)gnat ^ -1` is zero only when `gnat` is `-1`). To print the
input forwards the character is printed before the call, and to reverse it the
character is printed after the call returns, so the last character read is
printed first. The call stack does the job of the buffer. `simple.c` does the
same in a function `filter()`.

6. Short circuit operators instead of if statements

The author could not use `if`, because its ROT13 `vs` would be an undefined
function in the other version. Every decision is written with `&&` and `||`,
for example `(--tang&1) && (gnat = ...)` and
`!tang && noon!=-1 && (gnat&2) && (... putchar(noon))`. The right side only
runs when the left side is true. `simple.c` uses normal `if` statements.

7. One function for two jobs

`pain` both prints characters and, when its first argument is not zero, calls
`main` (for the ROT13 version where `znva` is the helper). In the original its
first argument is always zero, written as `tang^tang` and `(int)ABBA&2`, so it
only prints. It is called twice, once before the recursion (prints when not
reversing) and once after (prints when reversing). The `ABBA-0` and `noon+0`
are padding that keeps the text reversible. `simple.c` puts the two `putchar`
calls directly in `filter()`.

8. ROT13 by arithmetic

`((((int)gnat&31)+12)%26)+((int)gnat&32)+65` turns a letter into its ROT13
letter: `& 31` gives the letter's place in the alphabet (`A` and `a` are 1),
`+12 % 26` moves 13 places on and wraps around (counting from 0), `& 32` keeps
the lower case bit and `65` is `'A'`. The range check before it lists the two
halves of the alphabet separately (`'n'..'z'`, `'a'..'m'`, ...). `simple.c`
keeps the formula but checks for `'a'..'z'` and `'A'..'Z'`.

9. Misused parameter types

`main` declares `gnat` and `ABBA` as `char **` (where `argv` and `envp`
usually go) but uses them to hold a character and a true/false flag, with
`(int)` casts sprinkled in. The characters are compared to letters as pointers.
`simple.c` uses `int` variables.

10. Old style C

There are no `#include` lines, so `getchar` and `putchar` are implicitly
declared, `main` and `pain` have no return types or return values, and `main`
uses a K&R parameter list. The Makefile also has to define `trgpune`, `pune`,
`vag` and `znva` for the other versions. `simple.c` keeps the missing includes
but uses modern function definitions.
