# 2025/endoh1

Difficulty: Hard

1. Source code shaped as ASCII art with digits used as pixels

The whole file is drawn as a Nixie tube, and digits are scattered through the
code (mostly inside comments like `/*79879879879_7*/` and inside unused string
literals like `"[05469685-8515845785369290"`) so that running the program on its
own source lights up a digit shape. Those comments and characters do nothing
for the computation; they are only there to be "pixels" of the picture.

2. Hiding the `#include` inside a comment with a line splice

The bulb at the top is a comment `/**\` that continues across lines. The
backslash at the end of `/-MMXXV\` joins it to the next line, and `/*/` closes
the comment, so `#include<stdio.h>` looks like it is inside a comment but is
actually a real preprocessor directive.

3. Encoded string literal

`S` holds `"%e>BE?E/nw%e=:w/m%e:w\n"`. A loop `for(...;i++[S]-='\n';);`
subtracts 10 from every byte until it reaches the trailing newline (which
becomes the terminating zero). The decoded string is the ANSI escape sequence
`"\033[48;5;%dm\033[30m%c\033[0m"` (set a 256-colour background, black text,
print the character, reset).

4. Constants written as confusing arithmetic

Simple numbers are written as long expressions so they can contain the right
digits for the picture. For example `z=9*8+20*24/12*20-2-8*93` is `126` (the
grid row length), `5948/(+6)/6/+64/638-~-2` is `-1`, `90>>5>>6` is `0`,
`x+298/5/64` is just `x`, `+92*0*5+p+4/6` is `p`, `(0+8)/4+i+++(13)-((7))-8+0`
is `i++`, and `'*'*'-'` is `1890`. Character literals such as `'\f'*' '`
(`384`), `'\r'` (`13`), `'y'` (`121`), `'x'-z` (`-6`) and `z-'|'` (`2`) are
also used as plain numbers.

5. Operator precedence traps and unusual syntax

`F[7<<7+7]` looks like `7<<7` plus 7 but `+` binds tighter, so it is
`7<<14`. `i++[S]` is the same as `S[i++]` because `a[b]` is `*(a+b)`.
`&"\n"[k!=w]` points either at `"\n"` or at the empty string after it, so one
`printf` call prints a coloured cell, a newline, or nothing.

6. Work packed into `for` loop headers and the comma operator

Almost all the logic lives inside `for(...;...;...)` headers with empty or tiny
bodies. The main loop reads characters in its condition (`c=getchar()`) and
updates the line width in its increment (`w=++x<w?w:x`). The glow loop does its
real work in the increment part, and the printing loop calls `printf` in its
increment part while the body computes the colour.

7. Flattening two nested loops into one counter

The glow is spread over a 13 by 7 box of neighbours using one counter `i` from
0 to 90. The column offset is `i%13 - 6` and the row offset is
`(i/13)*2 - 6`, which is twice the real row offset so that the distance
`k*k+c*c` accounts for characters being about twice as tall as they are wide
(`k*63` then becomes the row offset times 126).

8. Packing several values into one integer

Each grid cell stores `character*126*126 + glow`. Dividing by `126` gives a
brightness level, and dividing that by `126` again gives back the lit
character (or nothing if it is not big enough). This avoids having separate
arrays for characters and brightness.

9. Hidden colour formula

The colour index is built in a tiny loop
`for(x=c-=c,k=z-'y';x<'\r';x+=k)c=(j<x?x:j<k+x?j:k+x)-x-c*~k;` which clamps the
brightness into three ranges of 0..5 and combines them as base-6 digits
(`-c*~k` is `c*6`). Then `c-~x` adds `x+1 = 16`, giving an xterm 256-colour
cube entry `16 + 36*red + 6*green + blue`.

10. Reused and meaningless variable names

Single letters are reused for unrelated purposes: `c` is `argc`, then the
current character, then a column offset, then a colour; `x` is the column and
also the colour loop counter; `k`, `j` and `i` change meaning between loops.
Unused variables `z3` and `a` are declared only to fill out the shape.

11. Hidden array margins

`p='\f'*' '+F` starts writing 384 cells (3 rows and 6 columns) into the array
so that the glow, which reaches 3 rows up and 6 columns left, never writes
before the start of `F`. Nothing in the code explains where `384` comes from.
