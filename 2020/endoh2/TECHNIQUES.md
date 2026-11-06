# 2020/endoh2

Difficulty: Hard

1. ASCII art layout with filler comments

The code is shaped into the big "1977 IOCCC2020" banner. Whitespace inside
expressions (`P=s*s*X[1]            +2        *s*t*`) and comments such as
`/**/`, `/*Stay_on_target.**/` and Star Wars quotes fill the gaps. Even the
`#include` lines are split by comments
(`#include/**    Do.Or ... --Yoda**/<stdio.h>`), so a simple search for
`#include <` doesn't find them. The compiler throws comments away, so they
change nothing. simple.c uses normal formatting and plain `#include` lines.

2. Compressed font hidden in a string

The letter shapes are packed into the string `G` (`"r2zZX!.+@KBK^yh..."`).
Each glyph is a run of characters ending with `!` (code 33). Each character
`c` holds a point number `c/2-18`, and its lowest bit (`c%2`) says whether
it is a bezier control point. A second string
`q="/&&&##%%##.+),A$$$$..."` lists the grid positions as differences: start
at 2 and add `*q-34` for each step to get the point's position `n`, then
`n%13` is the x and `n/13` is the y. To find letter number `n`, the code
skips `n` runs with `for(;n--;)for(;*p++>33;);`. simple.c gives the font as
a table of `{x, y, is_control}` points for each letter A to Z (the same data
as `obfuscation/glyphs.txt`).

3. Hidden default message

The text `"L@X-SGD-HNBBB-AD-VHSG--XNT"` is the default message
"MAY THE IOCCC BE WITH YOU". Each letter is stored one place too low (`@` is
A, `L` is M) because the glyph number is `*s-64`, and `-` (less than 63)
marks an empty column. The message is laid out in rows of 7 with `m%7` and
`m/7`. simple.c writes the message as plain text.

4. ROT13 hidden in a remainder

When a file is read, a letter's glyph number is `x%26`. Because `'A'` is 65
and `65%26` is 13, every letter is shifted by 13 places. That is the ROT13
cipher, so `URYYB` shows up as `HELLO`. The 26 in `%26` makes it look like a
normal alphabet index. simple.c writes `(ch - 'A' + 13) % 26` with a comment.

5. Complex numbers used as 2D points

The program uses the C99 `complex double` type (renamed `c` by `typedef`) as
an (x, y) point: the real part is x and the imaginary part is y. Several
tricks depend on this:
- Giving a complex value to a `double` or `int` keeps only the real part, so
  `s=P` takes x and `m=P` takes x as a whole number.
- Multiplying by `I` turns the point by 90 degrees, so `u=I*P` gives `-y`
  (the distance from the viewer) and `k=-I*P` gives `y`.
- `P*(2*s-P)`, where `s` is the real part of `P`, is the same as `P` times
  its mirror image (conjugate), which is x*x + y*y, the squared length.
  So no `math.h` functions are needed.

simple.c uses `creal`, `cimag` and `dx*dx + dy*dy` so it is clear which
part of the point is used.

6. Short meaningless names and shared global variables

Everything is one or two letters (`B`, `b`, `P`, `O`, `X`, `D`, `S`, `G`).
The globals `m`, `k`, `n` and `p` are reused for unrelated jobs: `m` is a
column counter while reading the file, then a loop counter, then a screen
column inside `b()`. `B` and `b` differ only by case. The bezier curve
(`X[3]`) and its offset (`O`) are globals instead of parameters. In
simple.c they have names like `project`, `draw_curve`, `struct curve` and
`letter_count`.

7. Flat integer array instead of a struct

Each letter is stored as three ints in a row inside `D[9999]` (x, y, glyph),
reached with `*d`, `d[1]`, `d[2]` and `d+=3`, or as `D[N*3]` for the x of
letter `N`. simple.c uses a `struct letter` array.

8. Logic squeezed into nested `?:` and comma expressions

The plotting code in `b()` is one statement made of nested ternaries:
`S[m]=S[m]-73?k%2?S[m]-94?95:73:S[m]-95?94:73:73`. The numbers are
characters: 73 is `I`, 94 is `^`, 95 is `_`. `x-73` means "x is not `I`".
Each screen cell is two dots tall: `^` is the top dot, `_` the bottom, and
`I` both. The choice between drawing a dot and splitting the curve in two is
also a `?:` whose other branch is `(b(...),b(...),0)`. simple.c uses `if`
statements and character literals.

9. Side effects packed into `for` and `if` conditions

The file loop `for(...;(x=fgetc(F))>0||fclose(F);)` closes the file inside
the loop test (`fclose` returns 0, which ends the loop). The test
`if(x>13?...,m++,0:1)` stores a letter as a side effect and is only true at
a line break (`x` is 13 or less). The centering loop
`for(++L;d-D>N*3||(m=0);N++)` counts the line and resets the column inside
its own tests. In the glyph loop, `if(*p%2?*a=*p,0:1)` saves a control point
and returns false in one expression. simple.c uses plain `while` loops and
`if`/`else`.

10. Screen buffer shared with the message string

`S[2000]` holds the default message (26 bytes), then the escape codes
`\x1b[2J` (clear screen) and `\x1b[H` (cursor home), and from `S+33` the
80x24 frame. The pointer `s` walks across the message while it is loaded, so
it ends up at `S+26` and the first `puts(s)` clears the screen. After that
`s=S+30` skips the clear. The plotting index `m+k/2*80+73` adds this 33-byte
start to the 40-column center offset. simple.c keeps the escape codes and the
frame in a separate `output` buffer with named offsets.

11. Unusual syntax

`int(main)(int(x),char**V)`, `clock_t(c)=`, `c(X)[3]` and
`typedef ... double(c);` put extra parentheses around names. They are legal
but look like function calls. `t*X` is written `t**X`, which looks like a
double pointer but means `t * X[0]`. `return+48*...` starts with a plus
sign that does nothing. `V[+1]` and runs of empty statements `;;;` are also
just noise.
