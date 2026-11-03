# 1994/schnitzi

Difficulty: Medium

1. Source code that is symmetric along its diagonal

The whole file is laid out on a grid so that it still compiles after its rows
and columns are swapped (which is exactly what the program does to its input).
Code that reads left to right in `schnitzi.c` reads top to bottom in the
flipped copy, so most of the program is written as one or two characters per
line, for example `main` on one line, `(` several columns later, `)` further on
and `{` after that. The compiler ignores all of that whitespace, but a human
reader has to hunt for each piece of a statement across the page.

2. Hiding code inside a giant comment

The second line starts `int r=0,x,y=0,    /*` and the comment does not end
until the ` */` many lines below, just before `n = 0;`. Everything in between,
including the ASCII art, a sideways `#include <stdio.h>` and sideways
`fseek`/`printf` calls, is ignored by the compiler. Those letters only become
real code in the flipped version, where they line up into horizontal text.
Splitting `int r=0,x,y=0, ... n=0;` across the comment also hides the
declaration of the column counter `n`.

3. ASCII art as camouflage

The comment contains pictures made from `/`, `\`, `|` and `-` characters. They
look like they might be operators, but they are only decoration that turns
into a hidden message when the file is flipped.

4. Meaningless single-letter names

`r` is the number of lines, `y` the longest line length, `n` the current
column, `x` the current line, `v` the read buffer, `s` the character being
printed, `t[]` the line lengths and `u[]` the file offsets where each line
starts. Renaming them shows what the two loops really do.

5. Reading characters by seeking instead of storing them

The program never keeps the lines it reads. During the first loop it only
saves each line's length and its starting position from `ftell(stdin)`. Later
it jumps back into the input with `fseek(stdin, u[x]+n, 0)` and reads one
character with `scanf("%c", &s)`. The literal `0` is really `SEEK_SET`
("count from the start of the file"), which is easy to miss.

6. Ternary used as a max function and `while` used as a `for`

`y=t[r]>y?t[r]:y;` is a compact way of writing "if this line is longer than
the longest so far, remember it". The outer loop `while(n<y){ ... n++;}` is a
counting loop whose initialisation is in the global declaration and whose
increment is hidden at the very bottom of the file.
