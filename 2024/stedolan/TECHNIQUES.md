# 2024/stedolan

Difficulty: Medium

1. Two nested loops packed into one counter

There is only one loop, `for(...;3-z/2160;x=++z%81/8-5)`. The counter `z`
walks over every character of the output, newlines included: 80 rows of 81
characters (80 pixels + `'\n'`). The program works out the row and column
from `z` with division and remainder. `z%81` is the column, `z/648`
(648 = 8 rows * 81) is which 8-pixel-tall block row we are in. The loop test
`3-z/2160` is a roundabout way of writing `z < 6480` (6480 = 80*81 = 3*2160).
The simple version uses ordinary `y` and `x` loops.

2. Hidden newline from an out-of-range column

The code never prints `'\n'` directly. `x` is the column divided into 8-pixel
blocks, minus 5, so the 80 pixel columns give `x` from -5 to 4. The 81st
character of each row gives `x == 5`, so `5>x ? pixel : 10` prints character
10 (newline) there. `x` also starts at 0 instead of -5 for the very first
character, which only works because the top row is always blank.

3. Mirror image computed with bit tricks

`16>>(x^-(x<1))+1` picks which input bit to show in each block column.
`-(x<1)` is -1 (all bits set) for `x <= 0` and 0 otherwise, and `x ^ -1`
is the same as `-x-1`. So for `x <= 0` it shifts 16 right by `-x`, and for
`x > 0` it shifts by `x+1`. Going across the 10 columns this gives masks
`0,1,2,4,8,16,4,2,1,0`. The left half uses bits 0 to 4 and the right half
copies bits 2, 1, 0 again as a mirror image. The edges give 0, which is the
blank border. The simple version writes this as a small lookup table.

4. Shifts that make a blank border for free

`G<<5>>z/648*5` picks the 5 bits for the current block row. Shifting left by
5 first means block row 0 sees five zero bits, so the top row is blank. Block
row 9 shifts past all 40 input bits, so the bottom row is blank too. The
program never tests for the border at all.

5. Precedence puzzles

The expression depends on C precedence that few people know by heart:
`+` binds tighter than `>>`, `>>` and `<<` bind tighter than `&`, and `!` binds
tighter than `^`. The author jokes about the "French quotes" `<<5>>`, the
"interrobang" `?!` and the "wink" `^-`, which are really two separate
operators side by side. The Makefile turns off `-Wparentheses` so the compiler
does not complain.

6. Character arithmetic for the output digit

`!(...)^49` turns the bit into the text `'0'` or `'1'`. `!` gives 1 when the
bit is clear and 0 when it is set, and XOR with 49 (the code for `'1'`)
changes 1 into 48 (`'0'`) and 0 into 49 (`'1'`). So set bits print `1`
(black in PBM) and clear bits print `0`.

7. Implicit int and missing headers

`long z,x,G;main()` uses globals that start at zero, `main` with no return
type, and `puts`, `scanf` and `putchar` with no `#include`. The Makefile adds
`-include stdio.h` and silences the old-style warnings. `%10lx` is also used
with a signed `long`, which needs `-Wno-format`.

8. Compression by mirroring

There is no table of pixels in the source. The 40 input bits become 64
picture cells because the right three columns of each row reuse bits from the
left side, and each cell is drawn as an 8x8 block, so the input is smaller
than the picture it draws.
