# 1987/hines

Difficulty: Easy

1. Spaghetti code made of `goto` statements

The program has no `while`, `for` or `else`. Every step is a label followed by
a `goto` to the next label, for example `tggo: ootg= strlen(tgoo); goto tgog;`.
The labelled pieces are written in a scrambled order, so the code that runs
first (`goog:` opening the file) sits near the bottom, and the reader has to
jump around the source to follow it. Rebuilding the order shows two simple
nested loops: an outer loop that reads lines with `fgets` and an inner loop
that walks along each line. In simple.c these are a `while` and a `for` loop.

2. Names made only from the letters g, o and t

Every variable and label is a shuffle of the letters in "goto": `togo` and
`toog` are `argc` and `argv`, `ogto` is the current position in the line,
`tgoo` is the line buffer, `ogot` is the file, `oogt` is the counter and
`ootg` is how many characters are left to check. The labels (`ggot`, `gtot`,
`ogtg`, `togg`, ...) are built the same way. Because all the names look alike,
it is very hard to tell a label from a variable or to remember which is which.

3. Constants hidden in variables

`otog=79` and `ottg=1` look like variables but are never changed. `ottg` is
used as the number `1` three times: `togo==ottg` checks for "no file name
given", `toog[ottg]` is the first argument and `exit(ottg)` is the error exit.
`otog` is the size passed to `fgets`.

4. `if` written as two `goto`s

Each decision is written as `if (test) goto a; goto b;`, for example
`if (!ogot) goto gogo; goto ggto;`. This replaces an `if`/`else` (or a loop
condition) with two jumps, so the reader cannot see which branch is the normal
path and which is the error or exit path.

5. Old style (K&R) declarations and missing prototypes

`main(togo,toog) int togo; char *toog[];` declares the parameter types between
the parameter list and the body, and `main` has no return type. `memcmp` and
`strlen` are used without including `<string.h>`, so the compiler has to guess
their types.

6. Dense formatting

The code is packed into a block where the tokens are lined up in columns with
extra spaces (`if (    togo==  ottg)   goto   gogo;`). The column layout looks
neat but it splits statements across lines and hides where each labelled step
begins and ends. The word `goto` also appears inside the output string
`"%d    goto   \'s\n"`, so the program counts its own `goto`s when run on
itself.
