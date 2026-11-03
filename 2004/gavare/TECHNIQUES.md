# 2004/gavare

Difficulty: Hard

1. No reserved words (implicit `int`)

The source never uses a keyword: no `int`, `if`, `for`, `while`, `return`,
`void` or `char`. Globals such as `E;S;C;D;` and `Q;T;U;u;v;w;` are declared
by just writing their name, and functions such as `F(b){...}` have no return
type. Old C assumed `int` for anything without a type, so every variable,
parameter and function is an `int`. This hides which names are variables and
which are functions, and what any of them hold.

2. Recursion instead of loops

Because `for` and `while` are keywords, every loop is a function that calls
itself. `I(x,Y,X)` tries one bit of the square root and calls itself with
`Y/2` for the next bit. `o(c,...)` tests sphere `c` and then calls `o(c+1,...)`
to test the rest. `t(x,y,a,b)` steps through the anti-alias samples with
`++a<A?t(x,y,a,b):(++b<A?t(x,y,0,b):0)`, `r(x,y)` calls `r(x+1,y)` for the
next pixel, and `s(y)` draws the rows. In `s` the line `r(0,--y?s(y),y:y)`
draws all earlier rows before the current one, so the rows still come out
from top to bottom.

3. Ternary and comma operators instead of `if` and statements

Since `if` is also a keyword, every decision is written with `?:`, and many
steps are packed into one expression with commas. The whole body of `n()`,
which follows a ray, is one huge expression of the form
`d>0&&Z>=0?(...reflect...):(...sky or ground...)`. Its branches contain dozens
of comma-separated assignments and nested ternaries such as
`V=V<22?7:(V<30?1:(V<38?2:(V<44?4:(V==44?6:3))))`. When nothing should happen,
the code writes `:0` as an empty branch.

4. Results returned through global variables

Without `return`, functions give back their results by setting globals. `F(b)`
puts the centre and radius of sphere `b` into `E`, `S`, `C` and `D`. `I()` and
`H()` put a square root into `E`. `q()` puts the hit distance into `p`. `o()`
puts the closest sphere and its distance into `Z` and `W`, and `n()` puts the
pixel colour into `Q`, `T` and `U`. The same global is often used for
unrelated things. For example, `E` is a sphere's x position in one place, a
vector length in another, and a dot product in a third.

5. Extra parameters used as local variables

Since local variables would need the `int` keyword, functions take more
parameters than they need, and callers pass `0` for the extra ones. `q(c,x,y,
z,k,l,m,a,b)` is called as `q(c,x,y,z,k,l,m,0,0)` so that `a` and `b` can be
scratch space. `n(...,d,a,b,V)` likewise uses `b` for the shading value and
`V` for the colour of the sphere that was hit.

6. Encoded data in string literals

The positions of the 44 spheres that spell "IOCCC ray" are stored as the
characters of two strings. Each character minus 64 gives one coordinate. For
example `'1'-64 = -15` and `'C'-64 = 3`. The strings are also cut into pieces
(`"1""111886:6:??AAF" "FHHMMOO..."`) that the compiler joins back together.
The height of each sphere is not stored at all. It comes from `b<22?9:0`, so
the first 22 spheres (the letters "IOCCC") stand upright and the rest
("ray") lie on the ground.

7. Fixed-point arithmetic with magic numbers

The program uses no floating point. Every coordinate is multiplied by
`M=1296` (and `N=36` is its square root), so `M` stands for 1.0 and products
have to be divided by `M` again, as in `x*x/M`. Constants like `P=9`
(maximum reflection depth), `O=255` (maximum colour), `_=1<<15` (top bit for
the square root) and colour gradients like `255-435*U/M` appear with no
names that explain them.

8. Confusing names and shadowed globals

Every name is a single letter, and some letters are used twice with different
meanings. The globals `X` and `Y` are the image width and height, but
`I(x,Y,X)` reuses `Y` as the current bit mask and `X` as the guess for the
square root. Inside that function the globals cannot be seen.

9. Bitmask colours

A sphere's colour is a 3-bit number in which the bit worth 1 means red, the
bit worth 2 means green and the bit worth 4 means blue, so `7` is white, `1` red, `2` green and `4` blue.
The lines `Q+=V&1?b:0,T+=V&2?b:0,U+=V&4?b:0` add the highlight only to the
colour channels whose bit is set.

10. Shaped source layout

The code is laid out so that the whitespace draws a picture, with
expressions split across lines at arbitrary points (for example
`b=x*x/M+y*y/M+z` / `*z/M-D*D*M`). It has to be reformatted before anyone can
read its structure.
