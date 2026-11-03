# 2000/briddlebane

Difficulty: Hard

1. Huffman-compressed word list

All 123 words (plus the "you are a " and " of " pieces) are stored in the
string `z` as a stream of bits instead of plain text. Function `k()` holds a
table of variable-length bit codes, one per letter, and `main()` reads bits one
at a time, growing the code in `A` until `k(A, l)` finds a letter with that
code and length. Common letters such as `e` get short codes (3 bits) and rare
ones such as `q` get long codes (10 bits), so the text takes less space and
cannot be read by looking at the source.

2. Code table packed into 2-bit symbols

The first 48 bytes of `z` are the code table for `k()`. Each byte is split
into four 2-bit pieces with `(z[F/4] >> ((F++%4)*2)) & 3`. After subtracting
1, the values 0 and 1 are the bits of a code, while -1 and 2 (both have bit 1
set, tested with `y&2`) mark the end of a code. The position of a code in the
table decides the letter: 0-25 are `a`-`z`, and 26-29 come from the bytes of
the number `2108973` (`0x202E2D`) masked with 47, giving `-`, `.`, space and
the string terminator `\0`.

3. Scrambled data bytes

The compressed bits after the table are not stored directly. Each byte is
first adjusted with `(z[...] + 114) ^ 2` before its bits are read, which is
what the README means by the "deliberate" numbers 114 and 2. The string also
mixes octal (`\253`), hex (`\x1d`) and letter escapes (`\b`, `\v`) and uses
`""` splits so the bytes look like noise.

4. Null-separated string table

The decoded text goes into the buffer `s` as 123 strings separated by `\0`.
`E(v)` finds string number `v` by skipping over `v` terminators with the
loop `w v--)w *(x++));`. Strings 0 and 1 are "you are a " and " of ", and the
rest form a word grid.

5. Words chosen by geometry

The words 2-122 are really an 11x11 grid. `f()` picks a random point in polar
coordinates (`sin`/`cos` of a random angle and a random radius), keeps
trying until the point lands on the grid inside a chosen ring, and turns
the point into a string number with `((y+5)*11)+x+7`. The inner disc holds
containers, the middle ring holds contents and the outer ring holds
adjectives, so a word's kind depends only on its distance from the centre.
Cells exactly on a border are never picked and just hold filler words.

6. Rings described by quadratic functions

The macro `e(t, v, w)` creates `a()`, `b()` and `c()`, which return
`(v - r) * (w - r)` where `r` is the distance from the centre. The product is
negative only when `r` lies between `v` and `w`, so `h(...) >= 0` in `f()`
means "outside the ring, try again". This hides simple `inner < r < outer`
tests behind multiplication.

7. Function pointers

`f()` receives the ring test as a function pointer `h`, so the same code is
used for all three word kinds and the reader has to track which function is
passed where.

8. Hidden call order

The three random words are chosen inside the `printf()` argument list. C does
not say in which order arguments are evaluated; this compiler does them from
right to left, so the contents word is picked first and the adjective last,
even though they are printed the other way round.

9. Odd random number scaling

`g()` divides `rand()` by `18972` instead of `RAND_MAX`, giving numbers up to
about 1.727 on systems where `RAND_MAX` is 32767. The angle is then
`g() * 6.283184` (about 2 pi) and the radius `sqrt(g() * 72)`, which are hard
to recognise as "random point in a circle".

10. Macros, typedefs and reused names

`#define w while(` and `#define r return` shorten keywords, and the typedefs
`u`, `q` and `n` hide `char`, `short` and `double`. Single-letter names are
reused for unrelated things: `x` and `y` are globals for coordinates and
decoded bits but also parameter names, and `B` and `C` are both globals in
`k()` and locals in `main()`.
