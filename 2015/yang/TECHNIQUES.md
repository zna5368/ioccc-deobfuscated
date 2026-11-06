# 2015/yang

Difficulty: Insane

1. Shaped source code layout

The whole program is laid out as a picture of a sea star, so statements are
broken over lines wherever the shape needs them, with extra spaces and empty
comments such as `void/**/w(int/**/_)` used as filler. Lines like `W( \` even
use a backslash to continue a line in the middle of a function call. Spaces and
line breaks mean nothing to the C compiler, so the shape only makes it hard
for a person to read.

2. Meaningless one letter names

Every global is a single letter (`I,bu,k,i,F,u,U,K,O`, `h,t,m,n,d,g,p,q,v,y,x`)
and functions are named `N`, `w` and `W`. Parameters are named `_`. In simple.c
these became `random_float`, `emit_char`, `emit_string`, `read_row`,
`write_row`, `rows_since_pattern` and so on, so each name says what it holds.

3. Reusing variables for unrelated jobs

`g` is a loop counter, the RNG mask, a column number and the last character
read, all in the same function. `x` is the column of the last sea star, but for
a moment it also holds "is this C?" (`x=0x1!=sizeof(' ')`) and then the "has
eyes" flag. `n` counts rows since the last pattern and is also used as an eye
column. A reader has to track which meaning is active at each point.

4. Language dialect detection through quiet changes

The program behaves differently depending on the C or C++ standard it is
compiled with. Four tests are hidden in macros and expressions:
- `sizeof(' ')` is 4 in C (a character literal is an `int`) but 1 in C++.
- `R(U)` uses `U"1"`. In C11 and C++11 that is one 32-bit string, so its
  element has size 4. Older dialects see the empty macro parameter `U` followed
  by `"1"`, whose element has size 1.
- `r(R)` uses `R"()"`. C++11 reads it as an empty raw string, so `[0]` is 0.
  Older dialects see the empty parameter `R` and then the string `"()"`.
- `x*=02//* */2` followed by `-1` on the next line. C99 treats `//` as a comment, so it
  becomes `x*=02-1`, which is 1. C90 has no `//` comments and sees `02 / 2 - 1`,
  which is 0.
In simple.c these are named macros (`IS_CPLUSPLUS`, `PATTERNS_ENABLED`,
`HAS_RAW_STRINGS`) and a function `has_line_comments()`.

5. Misleading macro and comment placement

`#define r(R) R"()"` sits at the very top before the includes, and
`/*[*/#include  /**/<stdio.h>` hides the include among comments. The
`/*!![crc=0f527cd2]*/` comment looks like it means something but is only a
checksum that the author placed in the source. The final `return /* "#*/0;`
places a quote inside a comment so that it looks like a string.

6. Bit tricks for simple arithmetic

`-~d` is used in place of `d+1` (because `~d` is `-d-1`), for example
`h=-~h%64` and `q=-~q*9%512`. `y^=20` and `y^024` switch between the letters
`p` and `d` because 112 XOR 20 is 100 and 100 XOR 20 is 112 (the ASCII codes of `p` and `d`).
`075` is the octal number for the character `=`, and `w(40)`, `w(41)`, `w(45)`
are the codes for `(`, `)` and `-`.

7. Hidden pseudo random number generator seeded by ASLR

`N()` is a linear congruential generator, `d = (d+1) * 1103515245` masked to
30 bits, written as `g=1<<30;d=-~d*1103515245&--g;`. The mask is made by
setting `g` to 2^30 and decrementing it in the same expression. The seed comes
from `d+=_[a]-(char*)0`, which adds up the memory addresses of the command line
arguments. Those addresses change on every run when address space layout
randomization is on. The signed multiplication overflows, which is undefined
behavior. gcc at `-O2` and higher uses that to produce different numbers in the
C90/C99 builds. simple.c uses unsigned math, which matches `-O0` and `-fwrapv`.

8. Ternary and comma operators in place of if statements

Most of the logic in `w()` is packed into expressions such as
`n=O+x?n=I+(x?0:N(k)-k/2),g=(t+42)%64,m=-~g%64,x?g=m=-~m%64:0,n>5?...:0,...:0;`
where `?:` acts as `if` and the comma operator chains many statements into
one. simple.c splits this into `draw_pattern()` and `draw_eyes()` with ordinary
`if` statements.

9. Self-recursive output function with lazy initialization

`w()` sets up its 64-row canvas the first time it is called (`if(t<0)`), and
when the next canvas cell is a blank or newline it prints that cell and calls
itself again with the same character (`if(g){w(_);}`) until the character
lands in a text cell. simple.c uses a separate `init_canvas()` and a
`do ... while` loop in `emit_char()`.

10. Ring buffer canvas with a flat array

`char o[5200]` is 65 rows of 80 characters, indexed by hand as `o[h*80+m]`.
Rows wrap with `%64`, so the canvas is a ring that is drawn into 44 rows
ahead of the row being printed. Each cell holds 0 (text goes here), a space
(part of a pattern) or a newline. simple.c wraps this in a `CELL(row, col)`
macro and named constants.

11. Shapes drawn with polar coordinates

The sea stars are made by the nested `for` loops over `u` and `K`. `u/5` is an
angle, `U` is a radius that waves five times per turn through
`i*.5*sin(u+F)` (giving five arms), and every cell along each ray is set to a
space. In C++ the arm length is 0, so a circle or dango is drawn instead.
simple.c names these `sweep`, `radius`, `arm_length` and `carve_spoke()`.

12. Data encoded as pointer distances in the generated program

The generated program starts with a table of 512 pointers in which slot
`(s+1)*9%512` points at slot `s`. Each input byte is written as the
difference between two slots, `o(p-(d=***(O***)p));`, where the number of
`*` dereferences decides how far through the cycle the pointer travels. The
main loop counts steps with `v` from 512 down until the distance matches the
byte in either direction, and swaps the names `p` and `d` with `y^=20`. A
table of 512 is needed so that every byte value from 0 to 255 can be reached in
at least one direction.

13. Strings split up and spread across the code

The header of the generated program is broken into many string pieces, such as
`"#include<stdio.h>typed" "e" "f\40int\40O;v"`, which uses `\40` for a space,
and the word `return` is split as `"r" "etu" /*J*/ "rn+0;}\n"`. The pieces are
joined by the compiler, but they cannot be read as one string in the source.
simple.c keeps them as `program_header` and `program_footer`.
