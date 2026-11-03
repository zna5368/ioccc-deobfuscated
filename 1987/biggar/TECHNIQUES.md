# 1987/biggar

Difficulty: Easy

1. Source code hidden in the compile line

The whole source file `biggar.c` is just `P;`. The real program lives in the
`Makefile`, where the compiler is given eight `-D` options such as
`-DP="main(){X}"` and `-DO="c=write(1,&c,1);"`. A `-DNAME=text` option works
like writing `#define NAME text` at the top of the file, so the preprocessor
turns the two-character file into a full program before the compiler sees it.
Reading `biggar.c` alone tells you nothing about what the program does.

2. Chained single-letter macros

Each macro expands into text that uses other macros: `P` becomes
`main(){X}`, `X` becomes `char c;W`, `W` becomes `while(C)I`, `C` becomes
`R>0`, `R` becomes `read(0,&c,1)`, `I` becomes `if(T)O`, `T` becomes `c!=015`
and `O` becomes `c=write(1,&c,1);`. To find the program you have to follow
the chain by hand, step by step, until no letters are left:
`main(){char c;while(read(0,&c,1)>0)if(c!=015)c=write(1,&c,1);}`.
The macros are also split in odd places (for example `C` holds only part of
the `while` condition), so none of them means anything on its own.

3. Octal character constant

The test `c!=015` uses the octal number `015`, which is 13 in decimal: the
ASCII carriage return character. Writing it as `'\r'` would make it obvious
that the program removes carriage returns.

4. Useless assignment

`c=write(1,&c,1);` stores the result of `write` into `c`, the character that
was just printed. The value is never used, because the next `read` overwrites
`c` straight away. It only adds noise and makes the reader wonder whether `c`
is being changed on purpose.

5. Implicit declarations and implicit int

There are no `#include` lines, so `read` and `write` are used without being
declared, and `main` has no return type. Old C compilers assumed that
anything undeclared returned an `int`, which lets the program compile while
hiding which library functions it relies on.
