# 1994/tvr

Difficulty: Hard

1. Macro shorthand for keywords and typedefs

`#define f for`, `#define M typedef` and `#define g [H]` hide common keywords
and the array subscript `[H]`, so `h g` really means `h[H]`. The typedefs
`b`, `c`, `d`, `e` and `_` rename `int`, `unsigned`, `unsigned char *`,
`XPoint *` and `void`, so you have to look up every type before you can read a
declaration.

2. Meaningless one letter names

Every global and function has a single letter name (`h`, `i`, `j`, `k`, `J`,
`K`, `L`, `Y`, `U`...). In simple.c they became names like `next_index`,
`orbit_stack`, `pixels`, `c_offset`, `build_map`, `iterate_unstable`,
`iterate_stable` and `draw_frame`.

3. Shaped source code and odd whitespace

The code is laid out as a picture with tokens spread over many lines (for
example `D [ B g = F ] ++` or the `gets(` call broken over five lines). This
hides where statements start and end.

4. Chained assignments in a macro to allocate memory

`P()` is `C N(h N(i N(s N(t N(A N(B=calloc(C-B+256,1)))))))` where
`N(a)` is `=r=(a)+j`. This is one long chained assignment that carves a single
`calloc` block into six arrays of `j` elements. Pointer arithmetic depends on
each pointer's type, so the same `+j` moves `j` bytes for the byte arrays and `4*j` bytes for the
`XPoint` and `int` arrays. The size of the
block is `C-B`, which comes from the previous call, so `P()` is called three
times: first to get a 256 byte line buffer, then to measure the layout, and
finally to really allocate it.

5. Nested conditional operators instead of if/switch

`J()` picks one of four fractal formulas with
`R-2?R?R-1?...:...:...:...`, and `main` picks the update function with
`(O<0?L:*("tvr@cs.hut.fi."+7*F)?F^1?Y:K:L)()`. In simple.c these are a
`switch` and an `if`/`else if` chain.

6. String used as a lookup table

`*("tvr@cs.hut.fi."+7*F)` looks like the author's e-mail address, but it is
used to test the animation method `F`. Indices 0 and 7 (`'t'` and `'h'`) are
non-zero and index 14 is the terminating `'\0'`, so method 2 always selects the
stable function `L`.

7. Calling a function chosen by an expression

The update function is chosen by a conditional expression that evaluates to a
function name and is then called with `()`. `(U)()` is just a call to `U()`
written in a way that looks unusual.

8. Algorithm hidden in lookup tables

The fractal formula (`z*z + c` and its variants) is never written in the main
loop. `J()` precomputes for every pixel the pixel index of the transformed
point and stores it in `h`. Adding the mouse offset `k` applies `+ c`, so one
iteration of the formula is just `h[R]+k`. Escaping the window is detected by
comparing that index as an `unsigned` value against `j`, which also catches
negative indices.

9. Animation by reusing the previous frame

`K()` never iterates a point more than once per frame: the count of a pixel is
the count of the pixel it moves to in the previous frame plus one
(`(A[R]<p)+A[R]`). Over many frames this slowly becomes the real Julia set,
which is what makes the "non-stable" modes animate.

10. Swapping buffers with arithmetic

`A += H -= H + H, B -= H` swaps the current and previous frame buffers. The two
buffers are `j` bytes apart, so `H` flips between `j` and `-j` and the pointers
move past each other. simple.c uses a temporary pointer instead.

11. Reusing variables and buffers for other purposes

The frame buffer `B` is also the line buffer used to read the colour map file,
the point buffer `t` is also the storage for `XNextEvent`, and `r` is a dummy
pointer used only to pass values along. The global `y` is used as the return
value of `I()`, and `v` changes from the static window to the animated window
after the first drawing.

12. Counter that is also a mode flag

`O` counts calls of the event loop condition `XPending(q)+!++O`. A mouse button
press sets it to `-42` so the next frame is computed with the stable function
`L()`. `L()` sets it to `-1`, so the next `++O` becomes `0` and `!0` forces a
blocking `XNextEvent`, which makes the program wait for the mouse after a
stable picture.

13. Redefining a standard function with a macro

`#define gets(B) fgets(...),(B[strlen(B)-1]='\0'),1` makes the call to `gets`
in `I()` read a line with `fgets` and strip the newline. The size passed to
`fgets` is `C-(B)+256`, using the end of the buffer block.

14. Hidden loop restarts

In `L()` the inner orbit loop changes the outer loop variable `R`, and the
`S` macro (`f(; G; )D[B[R=i[--G]]=F+=F<p]++`) quietly puts it back to the
starting pixel when it unwinds the orbit stack. The static Mandelbrot loop in
`main` is a single `for` with an `if` that both iterates and moves on to the
next pixel, which simple.c splits into two nested loops.
