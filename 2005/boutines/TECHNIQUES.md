# 2005/boutines

Difficulty: Hard

1. Macros that build code from fragments

`str(l,e,x)` expands to `l e,l x`, so it repeats its first argument twice. It is
used to write `const void*x,const void*y` in function headers, to produce both
coordinates `F+y,F+i` in the `<rect>` call, and, at the end of `main`, to print
each label twice: `str(o(...),&&r--,)` becomes `o(...) && r--, o(...)`. The
first copy draws a black label, `r--` switches the color index, and the second
copy draws the ivory label on top. `mx(p,v1,v2)` fills four array slots at
once, and `vi(n)` expands to `"width",n,"height",n`.

2. Renamed library functions

`std(i,scan)` and `std(o,print)` use token pasting (`y##f`) to declare function
pointers `i = scanf` and `o = printf`. Every call to `i(...)` or `o(...)` is
really a call to `scanf` or `printf`, which hides what the program reads and
writes.

3. One string used for two jobs

The array `_` begins with four spaces followed by the SVG header format string.
`4+_` is passed to `printf` to print the header, while the first three bytes are
overwritten by `a()` to hold an averaged RGB color that is returned as a
pointer. The same trick is used with `q`, which holds both the label colors
(`q+0` "ivory", `q+1` "black") and the SVG words "width", "height" and "text".

4. Function names that hide their purpose

Short names like `t` (squared distance), `ln` (site color), `put` (binary
search), `g` (nearest site), `a` (anti-aliased pixel color) and `tr` (draw a
square) tell nothing about what they do. Inside `g` a local array is also
named `g`, shadowing the function.

5. Comma operators and side effects in loop headers

The nearest site search in `g` is one `for` statement whose condition computes
which walker to move (`p=*(1+g)<*g`) and whether to keep going, whose body
moves the walker and computes its distance, and whose increment part updates
the best site with `s=*f<m?m=*f,*(r+p):s`. In `tr` the first `for` loop has an
empty body; all its work is checking corners in the condition.

6. Using `&&` and multiplication instead of `if`

The README boasts about having a single `if`. In `tr`, the decision "draw this
square or split it" is written as
`w=4*(w>3&&(h=...)&&o("<rect ..."))`: the rectangle is printed only if the
left side of `&&` is true, and the result (0 or 4) decides whether the
following loop recurses into the four quarters. In `main`, `0&o(...)` prints
the SVG header while passing `0` as an argument to `tr`.

7. Reusing variables for unrelated values

In `main`, `k` first holds `sizeof(double)`, is used as the record size in
`qsort` (`2*k`), and then becomes the label counter. `r` is the buffer size,
the sentinel coordinate, the count of `F` digits printed on bad input, and
finally the label color index. `v` is the number of values read, then halved
in the middle of the `qsort` call to become the number of points.

8. Hidden memory layout in one buffer

A single `calloc` block holds everything. Points are read into the bottom, a
reversed copy of the input is written from the top down (`*--f=*b++`) so
labels can later be printed in input order, and `ln` stores random colors
in bytes right after the sorted points with the pointer math
`(c*)(d+2*(2+v))+2*(p-d)`. Nothing in the code names these regions.

9. Pointer arithmetic instead of array indexing

Points are pairs of doubles and are reached with expressions like `*(f+(r*=2))`,
`*(1+g)`, `*(r+p)+=p?-2:2` and `*(b-1)`, which hide that the code is walking
over `(x, y)` pairs.

10. Encoded numbers and messages

`0x630C65u` is the number of `F` digits printed after `0x7` on bad input, so
the output is the Mersenne prime 2^25964951-1 in hexadecimal. `F` (15) is the
margin and `U` (512) is the picture size, so `2*F+U` is the 542 pixel SVG size
and `U**b` is `512 * *b` (multiply then dereference, not a power).

11. Misleading layout

The function bodies are indented with closing and opening braces pushed far to
the right, the `#include` lines are moved to the right edge, and a string
literal is split with a backslash-newline in the middle of `rgb(%d,%d,%d ...)`,
which quietly puts eight spaces into every output `<rect>` style.

12. Recursion in place of loops

`put` is a recursive binary search with a ternary choosing the next range, and
`tr` recursively splits the picture into quarters (a quadtree) until the four
corners of a square share the same nearest point or a single pixel is reached.
