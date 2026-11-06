# 2019/dogon

Difficulty: Insane

1. Picture layout

The code is shaped like ASCII art with big gaps of whitespace in the middle of
expressions, e.g.
`if(l<194&s>` on one line and `6|I>=H+` on the next. The compiler ignores the
whitespace, but a person has to glue the pieces back together to read a single
statement.

2. Single letter, reused and shadowed identifiers

Almost every name is one letter (`H`, `A`, `s`, `h`, `L`, `I`, `e`, `l`, `g`,
`p`, `C`, `n`, `w`...). Globals are reused for unrelated jobs: `I` counts input
lines and later holds the pressed key, and `D` counts input cells and later is
the image row stride. Names are also shadowed: `n` is the global hash table but
inside `u()` a local `o n` is a pixel pointer, `U()` has a local `int L` that
hides the global run mode `L`, and in `main()` the globals `f`, `j` and `m` are
used during input and then hidden by `Window f`, `XImage*j` and `XEvent m`
declared later in the same function.

3. Macros that hide syntax

`G` and `O` stand for `>>` and `<<`, `E(b)` is a `for` loop with the counter
`V`, `S` expands to a 4 iteration loop that also computes `k=V+(V&2)` followed
by a comma so the following statement becomes the loop body, `R` declares the
same set of local variables in every function, and `P(V,I)` is a hidden
recursive call `U(q+V,t*4+I)`.

4. Typedefs with meaningless names

`typedef unsigned long a,*T; typedef unsigned*o;` makes `a`, `T` and `o`
mean "unsigned long", "pointer to unsigned long" and "pointer to unsigned", so
declarations like `T p, C; o n, w;` give no hint about what the variables are.

5. Constants computed at run time

`unsigned F=-1` becomes `0x0F0F0F0F` after `F/=17`, and `y=-1` becomes
`0x1111111111111111` after `y/=15`. These masks are used throughout the bit
twiddling. `F/256` and `F/32` are also used as memory offsets
(`0x0F0F0F` and `0x787878`).

6. Magic numbers

`194` is the index of the first non-empty cell (`3*64+2`), `64` and `63` are
the node flag and step mask bits, `917` and `719` are hash multipliers,
`3855`, `15420` and `18` pick the middle 4x4 out of a leaf, `513<<12` is the
background color bits, `99` is the leaf border color and `8`, `27`, `29`,
`32`, `45`, `48` and `61` are key codes (the low 6 bits of the keysyms).

7. One memory block for everything

A single `calloc(Z,8)` holds the hash table, then the image pixels, then the
history stack, then the cell array `p`. The input line buffer is placed 99
bytes before the pixels and the numbered input cells are stored in the pixel
buffer itself (`w[++D]`) before the picture is ever drawn. `C` is just `p+1`
so `C[c]` is the second word of cell `c`.

8. Packed data structures

A node is three longs (two quadrant indices packed per long, plus the result
index, a flag and the time step packed in the middle long) and a leaf is a
64 bit 8x8 bitmap. Quarter leaves are stored in the low nibble of each byte of
an unsigned. The 16 element arrays `q[]` are used as a 4x4 grid so the four
quadrants of a cell are always at positions 0, 1, 4 and 5.

9. Bit parallel Game of Life

The leaf rule computes all 64 cells at once: shifting and masking with
`0x1111...` gives 16 four bit counters in one long, and the expression
`((B+y&y*6^3*y)+y>>3)&(d>>V|B)&y` applies Conway's rule to all counters
without any `if`.

10. One function with many jobs selected by flags

`U(v,j)` packs the level and two flags into `j` (`level*4+flags`). Depending on
the flags it returns a cell, its result, or the middle quadrant, and it also
does hashing, allocation and the whole hashlife recursion. The same flag trick
is used in `l=P(0,2*r)` where `r` decides if the loop keeps growing or takes
the time step.

11. Bitwise operators instead of logical ones

Conditions use `&`, `|` and `^` in place of `&&`, `||` and `!=`, for example
`if(C[c]&63^L|!c)` means "if the cell is new or its step changed" and
`(V^V/2)&5^5` is true for the 12 outer squares of the 4x4 grid. Operator
precedence has to be worked out by hand to read them.

12. goto based main loop

The main loop is built from the labels `d:` (print status) and `K:` (draw and
read keys) with `goto d` and `goto K` jumping between them instead of a
`while` loop. A fall through out of the shrink loop silently skips the
history and the printout.

13. Arithmetic in place of branches

Key handling is done with arithmetic, e.g.
`*(I&1?&H:&A)+=(I-1&2)-1 O s+2` picks the x or y variable through a pointer
and the direction with `-1` or `+1`, and `h+=(I==29)-(h&&I==27)` changes the
speed with comparisons used as numbers.

14. Format string mismatches and gets

`printf` uses `%d` for `long` values and `sscanf` writes `%d` into a `long`
and into `unsigned` variables, relying on little endian layout. The input is
read with the unsafe `gets()`.
