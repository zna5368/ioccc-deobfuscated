# 2024/tmarrec

Difficulty: Hard

1. Source code shaped into a picture

The whole program is spread over the page with tabs and spaces to draw a
picture, so statements are cut in the middle (`*W(t` on one line, `,I,a,E)` on
the next). The compiler ignores the extra whitespace, but a reader cannot see
where one statement ends and the next starts.

2. Single letter names for everything

Every variable and function is one letter: `W` returns a grid cell, `b` is a
fast square root, `H` normalizes a vector, `s` adds a source, `o` copies a
field, `u` advects, `q` projects, `v` subtracts a gradient. Even the camera
position (`T`, `B`), the camera axes (`R`, `w`, `Z`, `m`, `E`, `i`) and the
loop counters (`t`, `I`, `a`, `F`) give no hint of what they hold.

3. Magic numbers in place of sizes

The sizes are written as plain numbers in global variables: `L=551368` is
82*82*82 (one grid with a border), `c=15168` is 64*79*3 (the ray directions),
`M=6400` is 80*80, `J=949` is 79*12+1 (one screen row), `O[4426112]` is
`c+8*L`. You have to factor the numbers to see that they are grid sizes.

4. One big array for all data

`O` holds the ray direction of every pixel followed by 8 fluid fields
(velocity, previous velocity, density, previous density). The field is
picked by adding `T*L` to the index in `W(t,I,a,T)`, so the reader only sees
numbers 0 to 7 such as `u(1,4,3,4,5)` instead of names like `VEL_Y`.

5. Macro hiding the loops

`#define _(x)for(F=0;x>F;++F)` hides every loop behind `_(e)`, `_(L)` or
`_(5056)`, and the loop body then rebuilds x, y and z from the single counter
with `G=F%M;t=G%d+1;I=G/d+1;a=F/M+1;`.

6. Code added on the compiler command line

The main loop is `for(;Q)`. `Q` is defined in the Makefile as
`fputs("\033[H",stdout);`, so the loop condition moves the cursor to the top
left corner each frame (and is always true). `stdio.h` is also added with
`-include stdio.h` instead of an `#include` line.

7. Variable shadowing

Local variables reuse the names of globals and functions. In `u()` the locals
`F`, `j`, `l`, `i`, `c`, `e` and `J` hide globals with the same names, while
the loop `_(e)` still uses the global `e` and `F`. In the ray marcher `float
F` hides the loop counter, `float g` hides the jet angle, `s` hides the
function `s()` and `int q` hides the function `q()`.

8. Bit tricks on floats

`b()` computes a square root without `sqrt` by treating the float as an
integer: `k+(*(int*)&x>>1)` with `k=0x1fbb4000` halves the exponent, which is
roughly a square root. The same trick is written out again inline in `main`
to find the distance from the center of the grid.

9. Hidden math formulas

Sine and cosine are computed with Taylor series (`m-(U*m)/6+(U*U*m)/120` and
`1-w/2+(w*w)/24`), the camera is rotated with `x=T*.9998+B*.02`, the up axis
of the camera is a cross product typed out by hand, and the box is drawn with
a signed distance function made of `?:` operators.

10. Useless calculations

Some expressions do nothing on purpose: `m=g+(I/d)` adds `I/d`, which is
always 0 because `I` is smaller than `d`, and the rays are written with
`I++` inside the function call so each direction lands one slot later than
the pixel it belongs to.

11. Strings that hold more than one thing

`h[]` holds the 35 character density ramp (`"   ...(\\tjxuc...$"`) followed
by the escape sequence `"\033[38;5;141m+"`. The code copies `h[35+a]` for the
color code and later overwrites two of its digits with the shade, so one
string is both the brightness table and the screen cell template.

12. Bitwise operators as logic

Conditions use `&` and `|` instead of `&&` and `||`, for example
`(a++<1)|(d>x&y<d&n<d&x>0&0<y&n>0)`. The `a++<1` part forces the loop to run
at least once, which is a hidden do while loop.
