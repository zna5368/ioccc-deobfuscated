# 2005/vik

Difficulty: Hard

1. Recursive calls to `main()` as a function

`main()` is both the program and the ray caster. Its first `for` loop casts a
ray in direction `y[i]`/`y[i + 32]` and returns the distance to the nearest
wall in `j`. Calls such as `main(0, Y) > 200` pass `X = 0` so the second
(setup and render) loop is skipped and `main` only does the ray cast. The
caller sets the global `i` before the call because `main` cannot take a
direction argument. In simple.c this is the function `cast_ray(direction)`.

2. Single letter global variables reused for different jobs

Almost every variable is a one letter global that changes meaning. `c` is the
map row while parsing and the texture row stride while drawing, `m` is a loop
counter, a byte counter, a frame count and a time difference, `d` is both the
viewing angle and the number 92 used to make the letter `f` (`10 + 92`), and
`l` is a map character, a texture row, and a wall distance. The `struct`
fields are also called `a` to `e`, the same names as globals. simple.c gives
each job its own named variable.

3. Whole programs inside `for` loop headers and the comma operator

Nearly all of the work happens in the three parts of `for (...;...;...)`
headers, joined with commas, and many loops have an empty body. For example
the texture loader is a `for` with no body: the condition reads the next byte
with `fread` and builds a pixel, and closing the file ends the loop because
`fclose()` returns 0 and is multiplied into the condition. Function calls like
`XCreateSimpleWindow` are hidden as arguments of other calls
(`XCreateGC(K, L = XCreateSimpleWindow(...), 0, 0)`).

4. Nested `?:` operators instead of `if` statements

The movement code `n ? (i = ..., main(0, Y) > 200) ? e += ..., f += ... : 1 : 1`
is two nested conditions ("if a cursor key is held, and the wall is far
enough, then move"). The ray cast uses `k >> 8 ? 0 : (...) ? j = l, ... : 0`
to mean "if the hit is on the wall segment and closer than before, remember
it". `l >> 6 ? 4477904 << (l < 0) : t[c * l]` picks ceiling, floor or texture
color.

5. Macros that hide the math

`D(o,X)` is a 2D line intersection (Cramer's rule) used twice with different
arguments: once for the position along the wall and once for the distance
along the ray. `Q(o,X,Y,C)` adds one wall segment and ends in a comma so four
of them chain into one expression that is then glued to a `&&` test.

6. Tables computed at start up instead of literals

One loop fills three tables at once: the sine table `y` (each row of 256 is
already rotated by the viewing angle of one screen column, and adding 32 gives
the cosine), the default maze `D[m & 511] = 10 + d * !(1 & (m >> 3 & m))`
which is `'\n'` or `'f'` depending on two bits, and the default checkered
texture `r[m++] = m / 18 << (16 & (m >> 6 ^ m))`.

7. Encoded constants and bit tricks

Numbers are written as expressions of other variables: `J << 8` is the "no
hit" distance, `J / 3` is the start of the temporary wall list, `4 * J` is the
frame time in microseconds, `119 - 1[Y[1]]` tests for the letter `w` in `-w`,
`l > 54 && l < 96` tests for `A`..`i` because `l` is the character minus 10,
and `a + 1 & -1 >> 1` keeps the frame counter positive. Key states are
stored in `R[63 & keysym]` so the cursor keys become `R[17]` to `R[20]`.
`0 ** ++u` looks like a power operator but is `0 * (*++u)`, a way to move
the pointer forward while producing 0.

8. Array indexing written backwards

`1[Y[1]]` is the same as `Y[1][1]` and `2[Y]` is `Y[2]`, because `a[b]` is
just `*(a + b)` in C.

9. Hidden data layout

BMP pixels are packed by shifting each byte into a 24 bit value
(`n >> 8 | C << 16`) and the texture pointer skips the 54 byte header with
`+ 19` (18 pixels plus one spare slot). Animation frames are found by dividing
the file size by 12288 bytes (one 64x64 frame). Inner walls between touching
blocks are removed with an unexplained test using `128 * (B->d - G->d)`,
which only matches a wall at the same place facing the opposite direction.

10. Formatting with indent

The author ran `indent` over the code so that long expressions are broken
across many lines at odd places, which hides where each loop header ends.
