# 2011/zucker

Difficulty: Insane

1. Keyword macros

`#define E return` and `#define S for` swap two keywords for single letters.
Every `E` is a `return` and every `S(...)` is a `for (...)` loop, so the code
looks less like C at first glance.

2. Hidden PPM header

The text `P6 600 220 255\n` never appears in the source. It is stored in the
font string `J` as `x^H^XXHZZXHZ]]2`, with 40 added to each character. While
scanning the font for the first letter, any character that is not a marker gets
40 taken off and is printed with `putchar` until the first marker sets `t`.
Once `t` is nonzero nothing more is printed. Because the header is printed by
the font search, an empty text argument produces no header.

3. Default text hidden in the font

With no argument, the text to draw is the string `J` itself. Each character is
XORed with 5 (`(*e|32)^z` with `z = 5`), so `LJFFF%7544` becomes `ioccc 2011`.
Layout stops once the text is 24 units wide, so the rest of the string is never
drawn. When an argument is given, `1[z=0,k]` sets the XOR key to 0 while also
reading `argv[1]`.

4. Packed vector font

Each glyph in `J` is a marker followed by pairs of characters. Markers below
`'('` set the glyph width (`c - 40 + 6`) and move to the next character code,
and `'('` sets the character code directly from the next byte. Each stroke pair
holds bit fields: `/32` picks a negative line, a positive line or an arc,
`/8&3` and `&7` give x and y numbers. The program reads these with the
`?:` and comma operators inside a `for` header. In `simple.c` the font is a
decoded table of `LINE` and `ARC` entries.

5. Single multiply-add for all vector math

`A(a, b, c)` computes `a + b*c`. It is used for addition (`c = 1`),
subtraction (`c = T`, where `T` is -1), scaling (`a = Z`, the zero vector)
and stepping along a ray. You have to read the third argument to know which
one it is.

6. Global variables only, with side effects

There are no local variables. `G()` writes its result into the global `K`,
and `Q()` reads `K` right after calling `A()`. The distance function `D()`
leaves the closest point in `C`, the surface color in `M` and the query point
in `P`, and `main` reads these after the call. `O()` also stores the dot
product in `q`. The same globals hold unrelated values at different times:
`w` is a glyph width and later an angle, `b` is a font index and later a loop
counter, `L` is `argc` and later the pixel column.

7. Overloaded constants

`X` is 40. It is the character `'('` in the font decoder, the starting
"infinite" distance in `D()`, and the specular exponent. `T` is -1, used as a
start index and as the scale for subtraction. `H` (0.5) and `Y` (0.66) are
reused as unrelated factors.

8. Loops and comma operators instead of if and while

There is no `if` or `while`. Each `for` header holds the real work, chained
with commas, and the loop bodies are empty. For example, the ray march
`S(u=i=R=0;!R&&94>(u+=i=D(h=A(h,d,i)));R=i<.01);` steps the ray, calls
`D()`, adds up the distance and tests for a hit, all inside the loop header.
Ternary operators take the place of every `if`.

9. Odd array indexing and implicit casts

`1[z=0,k]` means `k[1]`, `b++[B]` means `B[b++]`, and `1[j]` means `j[1]`.
Floats go into `int` globals (`o`, `a`, `t`) to drop the fraction instead of
using a cast. `putchar` is called through the pointer `m`, and the color
floats are passed straight to it, which turns them into bytes.

10. Shared exit condition for the bounce loop

The reflection loop runs while `R*n + R` is nonzero. `R` is the hit flag and
`n` is the bounce count left, so this means "the ray hit something and bounces
are left". At the start the loop uses `g = R = 255 - (n = z)*64`, which sets the
color weight, the hit flag and the bounce count in one expression.

11. Shaped source layout

The code is laid out in the shape of a reflection ray hitting a surface.
Spaces and line breaks fall in the middle of expressions and even inside
`i\nnt`, which uses a backslash to continue a line.
