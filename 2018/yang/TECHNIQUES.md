# 2018/yang

Difficulty: Medium

1. Several programs sharing one block of text

The source is a square of characters that is a different C program when it
is read in each of its four rotations. Only a narrow band along the left
and top edges is the rotate left program. Every line of it ends with `//`,
so the rest of the line is a comment as far as this program is concerned.
The comment text is really the code of the other programs, which are read
down the columns or from the bottom right corner. For example the last two
lines contain `#define N(U,K,O)` and `int main()` written backwards as
`O,K,U(N enifed#` and `)(niam`, because that part is read upside down.
Strings such as `"jiu!fmcjefoJ"` and words like `SHOUJO`, `SHUUMATSU`,
`RYOKOU` and `IOCCC` belong to those other programs too.

2. Code that still works after one column is moved

The border of the square has two notches: the space at the start of the
line ` ;k--` on the left edge and the space in `Ip ;I` on the bottom edge.
Removing leading spaces after a rotation slides one row or column of the
square by one character into such a notch.
Characters are placed so that both the normal and the shifted text compile.
Because of this, many lines contain odd spacing and line breaks in the
middle of expressions, such as `d[n++` / `][u]=o` and `for(` / ` ;k--`.

3. A branch that never runs

`if(i){...}else{d[0][0]++;puts("Nuko");}` looks like it could print
`Nuko`, but `i` is declared as `i=1` and never changes, so only the first
branch runs. The else part exists for the other orientations of the code.
simple.c leaves it out.

4. Comments inside preprocessor lines

`#include/*nui*/<stdio.h>` has a comment between `#include` and the file
name. The compiler removes the comment, so it is a normal `#include
<stdio.h>`, but it does not look like one and simple text searches miss it.

5. Unused and misleading declarations

`extern int n0;` declares a variable that is never used. `typedef int x;`
creates the type name `x` so that `int` can be written as `x` in
`x n,u,k,o,_,i=1;` and `x main()`. The variable `_` is never used.

6. Numbers written as calculations

`64/2` is 32, the code of a space character, used to fill the grid.
`0x401` is 1025 and `0x0` is 0. `u=n=0/4` is just 0, `k/01` and `u/1` are
`k` and `u` divided by one, `!1` is 0 (the end of string character) and
`return+0` returns 0. The first array size is `1125` instead of 1024 only
to keep one compiler from warning about the loops.

7. Nested conditional operators instead of if statements

All input handling is hidden in the increment part of the reading loop:
`u+=o-10?n<1024?d[n++][u]=o,k=k<n?n:k,0:0:!(n=0)`. `o-10` is not zero for
every character except a newline (code 10). For a normal character it
stores it, moves to the next column, updates the longest line `k` and adds
0 to `u`. For a newline `!(n=0)` sets the column back to 0 and evaluates to
1, which moves `u` to the next line. simple.c uses an `if`/`else if`.

8. Comma operator and work in loop headers

The output loop `for(;k--;)d[k][u]=!1,puts(d[k]);` counts `k` down inside
its condition, and the body ends the row with a 0 and prints it in one
expression joined by a comma. simple.c counts down in a normal `for` loop
with two statements.

9. Transposed storage

The character at column `n` of input line `u` is stored in `d[n][u]`, so
the array rows are the input columns. Printing the rows from the last one
to the first gives the text rotated counterclockwise, without any explicit
rotation code. simple.c names the array `grid` and the indexes `column`
and `line`.

10. Short, meaningless names

`n` is the column, `u` is the line, `k` is the longest line, `o` is the
character read and `d` is the grid. simple.c uses descriptive names and
named constants `MAX_COLUMNS` and `MAX_LINES` for 1024.

11. Code formatted into a picture

The program is drawn as a square frame made of `//` and `/` characters
with a ring of `@` in the middle, the date `(c)2018` and the CRC32 of the
file `[cffc189a]` hidden in the last line.
