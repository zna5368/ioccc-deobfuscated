# 2005/vince

Difficulty: Medium

1. Data hidden in the program's own source code

The program opens its own source file (`fopen(__FILE__, "r")`) and reads it
one character at a time with `G` (`fgetc(F)`). `while (a != 89) a = G;` skips
ahead to the first `Y` (ASCII 89), which is the name of the function `void
Y()`. `while (a != 81) a = G;` does the same for `Q` (ASCII 81) and `void
Q()`. Two more `G` calls skip the `()`. The rows of braces after those names
are not code that does anything. They are a picture made of characters,
and the program reads that picture as data. simple.c stores the decoded
data in the arrays `texture_art` and `cube_faces`, so it no longer needs to
open its source file. This means error 4 (can't open the source file) cannot
happen in simple.c.

2. Brace art used as a bitmap

The texture loop runs 256 times (once for each pixel of a 16x16 image). Each
time it reads two characters (`G; a=G;`) and keeps only the second. If that
character is whitespace (`a<33`) the pixel is grey (128,128,128), otherwise
it is dark red (128,0,0). The red is made with `128 & -(a<33)`: `-(a<33)`
is `-1` (all bits set) or `0`, so the `&` either keeps 128 or clears it.
Reading the brace art in `Y()` this way gives a picture with "2005" in it.
The empty `{}` blocks are valid C, so the art still compiles.

3. Brace art used as cube coordinates

`Z(x)` reads two characters and turns the second into 0 or 1 with
`x<50-(x==59)`: whitespace (below 50) becomes 1 and braces or `;` become 0.
`R()` uses `Z` three times to read a 3D point `c,m,n`. For each of the six
faces the program reads a normal and then four corners (`V` is
`R(); glVertex3i(c,m,n);`) from the art in `Q()`. Only 0 and 1 can be
stored, so the three faces that should have a normal pointing the negative
way get a zero normal. simple.c lists the decoded faces in `cube_faces`.

4. Abusing the size rule for whitespace and `{ } ;`

The IOCCC size rule does not count whitespace, or `{`, `}` and `;` when they
are followed by whitespace. The author stores all the data as whitespace,
braces and semicolons, so the texture and the cube cost almost nothing
toward the size limit.

5. Short macros for long OpenGL names

Macros such as `H` (`glEnable(`), `I` (`glMaterialfv(GL_FRONT,`), `J`
(`glMatrixMode(`), `U` (`glLightfv(GL_LIGHT0,`), `P` (`glCallList(u);`),
`T` (`glTexCoord2i`) and `X` (`GL_TEXTURE_2D`) shrink the long OpenGL names.
Many of them leave a bracket open, so the closing `)` is far away from
the opening one. simple.c writes the full function calls.

6. Macros with unbalanced brackets

`L` is `(y`, the display variable with an open bracket in front, so
`XOpenDisplay ( N ) ) )` and `glXQueryExtension L , N , N ) )` only balance
after `L` is expanded. `M` is `640,480`, which is two arguments, so
`XCreateWindow L, root, M, M, ...` passes four numbers (x, y, width,
height). `O` is `GL_DIFFUSE , i`, also two arguments.

7. Error codes printed with a goto

`E(x)` prints a number and jumps to the label `l:`, which is just before
`return 0;`. The name `l` is also used for an unused global `int l`;
labels and variables have separate names in C, so both can exist. The same
macro prints `0` when a key other than space is pressed. simple.c uses
`printf` followed by `return 0` at each place.

8. Reusing variables for unrelated jobs

`x` is first the texture index, then the face loop counter. When the loop
ends it is 6, and the animation then uses that leftover value as its speed:
`g+=.07*x` turns the cubes by 0.42 degrees and `h-=x/100.0` moves the second
cube by 0.06 each frame. `c` is a cube coordinate while the display list is
built and later the index of the light colour to toggle. All four light
values and the ambient colour live in one array `i`, and `i+4` and `i+8`
point into the middle of it. simple.c uses separate, named variables.

9. Key test with a bit mask

`XLookupKeysym(...) & 1<<5` checks if bit 5 (value 32) of the key symbol is
set. The space bar is 32, so it passes. Many other keys also pass, for example
lowercase letters. Keys such as Escape do not, and they end the program.
`c++; c&=3;` counts 0, 1, 2, 3, 0, ... by keeping only the two lowest bits.

10. Duplicate declarations and odd layout

`n` is declared twice in the same global declaration
(`int u,t,l,m,n,...,o,n,c;`). At file scope C treats this as the same
variable declared twice, which is allowed. The code of `main` is laid out
as a shape with spaces and tabs, with statements split across lines in odd
places, such as `E(` at the end of one line and `4 ) ;` on the next.
