# 1994/smr

Difficulty: Easy

1. Empty source file

`smr.c` contains zero bytes. There is no code at all to read, so the "trick" is
that the program's entire behavior comes from what is missing. When an empty
program runs it prints nothing, and nothing is exactly what the source file
contains, so its output is a perfect copy of itself. This makes it the smallest
possible self reproducing program (a "quine").

2. Build rule that avoids the C compiler

Most C compilers refuse to link an empty file because there is no `main`
function. The entry's `Makefile` gets around this by not compiling at all: the
`smr` rule just copies `smr.c` to `smr` with `cp` and marks it executable with
`chmod +x`. When you run `./smr`, the operating system sees a file that is not a
real binary and hands it to the shell, which runs an empty script and exits
successfully with no output. The program looks like C but is really run as a
(blank) shell script.

3. Exploiting a loophole in the contest rules

The entry relies on the fact that an empty file is neither clearly a valid nor
clearly an invalid C program. The contest rules at the time only set a maximum
size, not a minimum, so a zero byte program was allowed. The judges later added
a minimum size rule to close this loophole.
