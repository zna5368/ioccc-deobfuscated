# 1993/lmfjyh

Difficulty: Easy

1. Program code hidden in the file name

`lmfjyh.c` is only `char*_=__FILE__;`. The real program lives in the name the
Makefile gives to the copy it compiles: `";main(){puts("Hello World!");}char*C=".c`.
Nothing in the source file itself says what the program does, so you have to
read the build command to find the `main()` function.

2. `__FILE__` used to paste code into the program

`__FILE__` is a preprocessor macro that turns into the name of the file being
compiled, written as a string. Compilers before gcc 2.3.3 did not put a `\` in
front of the `"` characters in the name, so the line became
`char*_="";main(){puts("Hello World!");}char*C=".c";`. The first quote in the
file name closes the string, the code after it is compiled as normal C, and
`char*C="` at the end opens a new string that swallows the leftover `.c`.
Modern compilers escape the quotes, so the whole name stays one string and
there is no `main()` to link.

3. Throwaway variables to balance the quotes

`_` and `C` are never used. They only exist so the quotes at the start and end
of the file name have somewhere to go: `_` gets the empty string in front of
the code and `C` gets the `.c` after it.

4. Implicit `int` and missing return

`main()` has no return type and no `return` statement. Old C assumed `int` when
the type was left out, which saves a few characters in the file name.
