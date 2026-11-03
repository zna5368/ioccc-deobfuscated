# 1989/fubar

Difficulty: Medium

1. Shell script and C program in one file

The source has no `#!` line, so when it is executed as a command the shell
reads it as a script. To the shell, every line starting with `#` (`#include`,
`#define`) is a comment. The only line both languages run is
`cc ouroboros.c -o x`. The shell sees a command that compiles the file. The C
compiler sees three macros `cc`, `ouroboros` and `x` placed next to each other,
plus the leftover tokens `.c` and `-o`. The rest of the shell script comes
after `#define zxc ;{/*`. For the shell that line is a comment. For C the `/*`
starts a block comment that hides the shell commands until the closing `*/`
at the end of the file. In simple.c the shell half moves to a separate
`simple.sh` driver script.

2. Self-modifying source code

The program never computes the factorial in a loop. The answer is stored in
the source as `#define TT 1` and the counter as `#define QQ 1`. Each run reads
its own source on stdin and prints a copy where `QQ` is one larger and `TT`
is multiplied by `QQ`. The shell part saves the copy as the new
`ouroboros.c` and runs it with `exec`, which compiles it again. When `QQ`
reaches the argument plus one, `TT` holds the factorial. The program prints it
and exits with a non-zero status, which stops the shell from starting another
pass. simple.c keeps this design but names the macros `COUNTER` and
`FACTORIAL`, and simple.sh runs the passes in a plain `while` loop.

3. Program body split across macros

`main` is built from three macros that each hold part of a statement: `cc` is
the function header and variable declarations, `ouroboros` is the "are we
done?" test, and `x` is the copy loop that ends with the closing `}` of
`main`. None of these is a whole statement on its own, so you have to paste
them together in your head to see the function. The `#define x` line is so long
that it continues onto the next source line, which only works after the two
lines are joined (fubar.sh does this with `ex` before running the file).

4. Junk expression to absorb leftover tokens

After the macros expand, the file name `ouroboros.c -o x` becomes
`o=c+f` `.c -o` `;while...`, which reads as `o = c + f.c - o;`. The struct
`f` with a member named `c` exists only so that `.c` is legal C. The value is
thrown away because `o` is overwritten by `getchar()` right after.

5. Matching text by character pairs and fixed widths

The copy loop remembers the previous character in `q` and looks for the
letters `QQ` and `TT`. When it finds them it uses `fread` to read a fixed
number of characters (6 and 11) holding the old number, then prints the new
number with matching widths (`%6d`, `%9d`) so the rest of the file stays
lined up. The flags `l` and `m` make sure only the first `QQ` and `TT` (the
`#define` lines) are changed. simple.c instead reads whole lines and uses
`sscanf` to recognise the two `#define` lines.

6. Meaningless names

The variables `q`, `o`, `l`, `m`, `tt`, `qq`, `c`, `v` and the macros `QQ`,
`TT`, `cc`, `x`, `zxc` tell you nothing about what they hold. `QQ` is the
counter, `TT` the running factorial, `tt` and `qq` the text of the old
numbers, `l` and `m` "not replaced yet" flags, and `zxc` is never used at all.
