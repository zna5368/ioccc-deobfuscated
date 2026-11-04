# 2005/persano

Difficulty: Hard

1. Code shaped like a knot

Most of the program is typed into the shape of a torus knot drawing. Statements
are broken in the middle of names, numbers and operators, such as `w=f` /
`(A)` and `-e*e`, and whole functions (`N`, `K`, `M`, `G`, `W`, `E`) are packed
into the picture with no line breaks between them. You have to reformat the
code before you can even see where one function ends and the next one starts.

2. Short typedef and macro names

`typedef double R, V[3], U[4]` hides the types: `R` is a `double`, `V` is a 3D
vector and `U` is one pixel (depth plus three colours). `Z` is `void`, `_` is
`int`, and `J` is a pointer to a function like `cos`. `X` is `for`, and
`F(v,b)` is a loop that sets all three parts of a vector. With these names
`Z N(R*v)` really means `void normalize(double *v)`.

3. Math functions hidden behind pointers

`J f = cos, Y = sin;` and `(*u)(_) = putchar` store library functions in
variables, so the code calls `f(t)`, `Y(A)` and `u(59)` instead of `cos(t)`,
`sin(A)` and `putchar(';')`. A reader searching for `sin` or `putchar` finds
nothing. `Y` is also used as an `int` local inside `E`, which hides the global
`Y` there.

4. Strings printed backwards with a character offset

`W()` is recursive. It prints the rest of the string first and then the current
character, so the string comes out reversed. Any character above `'a'` has 98
subtracted, so `'b'` becomes byte 0, `'c'` byte 1, `'e'` byte 3 and `'i'`
byte 7. So `"bb\346cbcba98FIG"` is the GIF header `GIF89a`, 256 x 256, flags
`0xE6`. `"ibcbcbbbbb,"` is the image descriptor, and
`"bbbce0.2EPACSTEN\x0b\xff!"` is the `NETSCAPE2.0` extension that makes the
animation loop. The byte `\346` is a negative `char`, so the `> 97` test does
not change it.

5. One generic loop function with a callback

`M(a, s, m)` sets `*a` to 0, calls `m(i)` `s` times and adds one step to `*a`
after each call, so `*a` goes once around a full circle. It is used three times
with three different meanings. `M(&A, frames, E)` turns the knot a little more
in each animation frame. `M(&s, 80, G)` walks along the knot curve, and
`M(&t, 20, K)` walks around the tube. Results are passed back through globals
(`A`, `s`, `t`, `o`, `d`, `v`, `q`), not through parameters.

6. Hidden π

The step is `pow(25/log(36),4)/s/377`. `(25 / ln 36)^4 / 377` is about
6.28318, which is 2π, so the step is a full turn split into `s` parts. This is
the author's third exercise.

7. Magic numbers built from `e = 256`

`e` is the image size, and the other constants are made from it: `e/2` is both
the image centre and the LZW clear code 128, `e*e` is the depth offset 65536,
`e-2` is the 254 byte block limit and `e-1` is the last 8-bit LZW code. Other
bare numbers are ASCII codes: `46` is `'.'`, `59` is `';'`, and `129` is the
LZW end code.

8. Palette built from a string of shift amounts

`u(k/3<<("gdb"[k%3]-97)|31|(k%3!=1)<<5)` prints 384 palette bytes. `"gdb"` minus
`'a'` gives the shift amounts 6, 3 and 1 for red, green and blue. Only the
lowest 8 bits are written, so bits shifted past the top are cut off. Each
palette index ends up as 2 bits of red, 3 bits of green and 2 bits of blue.

9. Recursive dither matrix

`T(p, k, n)` fills the 8x8 Bayer dither matrix `D`. The quadrant offset
`9 & (g*4|g)` gives 0, 1, 8, 9 (right, down, and diagonal in a row that is
8 wide). The rank `(4-g)&3` gives 0, 3, 2, 1. Each level of the recursion adds a
smaller weight `1<<6-2*k`. The parameter `k` hides the global loop counter `k`.

10. Global loop counter shared by every function

The global `k` is the loop variable in `N`, `K`, `G`, `E` and `main`. The `F`
macro also uses it without saying so. `main`'s palette loop `X(;k<384;k++)` has
no start value at all. It only works because `k` is a global that starts at 0,
and `T` uses its own parameter `k`.

11. Indexing written backwards

`1[v]`, `2[v]`, `k[v]`, `k[o]`, `h[x]` and `h[d]` mean `v[1]`, `v[2]`, `v[k]`,
`o[k]`, `x[h]` and `d[h]`. In C `a[b]` is the same as `*(a+b)`, so the order
can be swapped.

12. Conditions made of arithmetic

Branches are written as sums with true/false values instead of `if`. The knot
point is `(k<2)*(x+3)*(k?y:z) - !(k-2)*w`. The light for each colour is
`*n*!!k + n[1]*(k<2) + n[2]`, and the square root `sqrt(2+!(k-1))` picks
√2 or √3. Each channel's colour depth is `3|!(j-1)*4` (3 or 7), and its bit
width is `k/4+2`. The clamp `(m|-(m>k))&k` ORs in all bits when `m` is too
big, so the `&k` then gives the maximum value.

13. Packed loops with side effects

The rasteriser in `E` puts almost all of its work into `for` headers and comma
expressions. The quad corner order comes from `(k^k/2)&1` and `k/2&1`. The
edge test is `k && y<*a ^ y<*b`. The span loop
`X(q=I+(_)*x; *l&&w--; q+=2*(*x<*l)-1)` steps left or right, and its body is
hidden in the start part of an inner `for`. The pixel output loop writes the
pixel and the clear code in the step part:
`i++, O(*r?c:127), g=g<e-1?g+1:(O(e/2),130)`. The pointer `r` goes down from
`&I[i][3]` while the colour is built, so at the end `*r` is the depth that
tells if the pixel was drawn.

14. Short-circuit operators as `if`

`O()` writes a full GIF data block with
`(S>e-2||c==129) && (*B=S, S=fwrite(...)-1)`. `fwrite` returns 1, so the
length is reset to 0 by the same expression that writes the block. `W()` uses
`*s && (...)` to stop at the end of the string.
