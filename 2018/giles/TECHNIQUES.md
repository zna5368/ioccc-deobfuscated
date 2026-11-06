# 2018/giles

Difficulty: Hard

1. Lookup table hidden as a polynomial

The `V[]` array looks like a list of 64-bit integers, but each value is the
raw bit pattern of a `double`. `Ll1(i)` reads them with `*(double*)(V+l)` and
evaluates a degree 15 polynomial using Horner's rule
(`Ll = Ll*i + V[l]`). The polynomial was chosen so that, for each input
0..15, the result truncated to an `int` is the next state of a 2x2 block of
cells. `simple.c` replaces it with the 16 entry table `block_rule[]`.

2. Bit-packed cell neighbourhood

`_L()` packs four cells into one number: bit 1 is top-left sand, bit 2 is
top-right sand, bit 4 is bottom-left occupied and bit 8 is bottom-right
occupied. The `M(a,b)` macro unpacks the answer with `LL/b&1`, where `b` is
1, 2, 4 or 8, and keeps the wall bit with `&2`. `simple.c` names the four
cells and shifts each bit out explicitly in `update_block()`.

3. Macros that hide syntax

`S(a,b)` expands to `b*a` and `N(a,b)` expands to `b=a`, so arguments appear
in the wrong order. For example `S(j=fopen,FILE)(...)` becomes
`FILE*j=fopen(...)` and `N(Ll1,LL)(...)` becomes `LL=Ll1(...)`. `S(i+,Ll= Ll)`
even splits an expression in two to make `Ll=Ll*i+`. `#define L lL[L1` leaves
an open bracket, so `L+l]` turns into `lL[L1+l]`.

4. Token pasting to shorten SDL names

`G(_)` pastes `SDL_` in front of a word, and `GX`, `GY` and `GZ` add
`Create`, `Destroy` and `Render`. A long list of one-letter macros (`G1`
to `GW`) then names every SDL function and constant used, so `G6(...)`
is really `SDL_CreateWindow(...)`. `GB(__,_G,G_,GG)` calls two functions in a
row with swapped arguments, for example `SDL_RenderPresent(r); SDL_Delay(40);`.

5. Lying constants

The globals `ONE = 1` and `ZRO = 0` are labelled "important constants", but
`int GK= GO+ (ONE= ZRO ++);` sets `ONE` to 0 and `ZRO` to 1 before the main
loop runs. Every later use is backwards: `v==ZRO` checks for sand (1) and
`GK=...?ONE:GK` stops the loop by setting it to 0. `-(ZRO-O)*10` is just
`(O-1)*10`. `simple.c` uses plain numbers and named constants instead.

6. Confusing identifier names

Variables use only the letters `l`, `L` and the digit `1` (`l1L`, `Ll1`, `l1l`,
`lll`, `ll1`, `_L`), which look almost the same. `l1L` is the step phase,
`l1l` is the row width, `lll` is the last row offset and `ll1` is two rows.
They have been renamed `phase`, `width`, `last_row` and so on.

7. Hidden alternating block grid (Margolus neighbourhood)

`JJ()` flips the global `l1L` each step and uses it as both the starting row
offset (`l1L?l1l:0`) and starting column, so the 2x2 blocks shift by one cell
diagonally every other step. The same global is then written into the top
row (`P[k]=l1L`), which only adds a new grain of sand on steps where the
phase is 1. Reusing one variable for two jobs hides both.

8. Shaped layout and split tokens

The code is drawn as a bucket pouring sand. Long lines are broken with
backslash-newline in the middle of words, such as `pri\ntf` and
`_\_LINE__` inside the `GA` error macro. Empty macros like `GU` and odd
spacing are put in to fill out the picture.

9. Data file encoding

The grid is read straight from a text file with `getc(j)-96`, so the
character `` ` `` becomes empty (0), `a` becomes sand (1) and `b` or `c`
become walls (bit 2 set). Newlines turn into negative numbers that still have the
wall bit set, so they act as invisible walls at the end of each row.
