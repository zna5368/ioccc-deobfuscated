# 2020/kurdyukov2

Difficulty: Hard

1. Token pasting macros that build library names

`#define J(x) jpeg_##x;` glues `jpeg_` to the front of a word and adds a `;`,
so `J(create_##c(I=&D))` with `c` set to `decompress` turns into
`jpeg_create_decompress(I=&D);`. The `W` macro goes further: its arguments
`decompress`, `src`, `read` and `output` (or `compress`, `dest`, `write` and
`next`) are pasted into `jpeg_stdio_src`, `jpeg_read_scanlines`,
`D.output_scanline` and so on. One macro body is used for both reading and
writing the JPEG, and no full library function name appears in the source.
Even `#r` turns the bare word `rb` into the string `"rb"`. `simple.c` writes
out the two blocks of libjpeg calls separately.

2. Code passed into macros as arguments

The real work is hidden inside the arguments of the `W` macro. The fifth
argument of the first `W(...)` is a long list of statements (get the image
size, clamp the rectangle count, allocate memory) that the macro drops into
the middle of the JPEG reading code. In the same way `B(e,f)` takes a
statement `e` to run first and an operator `f`: `B(...,-)` makes
`d[...] - X->c[j]` to measure the error, and `B(,=)` makes `d[...] = X->c[j]`
to paint the rectangle. `simple.c` uses the functions `update_rect` and
`paint_rect` instead.

3. Macros and line breaks that split words

Words like `for`, `_space` and `struct` are cut in half with a backslash at the
end of the line (`fo\` + `r`), and macro calls like `W(` and `H(` start on one
line and end on another. This makes the code hard to search and read.

4. One macro used for two directions

`H(W,H,w,h,B,i)` tries every place to cut a rectangle in two. It is called as
`H(b,S,v,q,e,k)` to try vertical cuts and `H(S,b,q,v,f,l)` to try horizontal
cuts. Swapping the "step one pixel right" (`b`) and "step one row down" (`S`)
numbers, and the width and height, turns one direction into the other.
`simple.c` has a single `best_split` function with named parameters
`step`, `cross_step`, `length` and `span`.

5. Summed-area table hidden in a one line loop

`for(x=j-S,z=j-b;x<S*h;x+=b)t[x+S]=x<0?y=0:t[x]+(u=y--?u+d[z+=b]:(y=w,0));`
builds a table where each entry is the total of all pixels above and to the
left. A negative `x` means "first row, write 0". `y` counts down the pixels
left in the row: when it runs out it is reset to `w` and the running row total
`u` is reset to 0. With this table the total of any rectangle needs only four
reads (`p[w*b+h*S]-p[w*b]-p[h*S]+*p`). `simple.c` has
`build_summed_area_table` with normal nested row and column loops.

6. One memory block shared by three arrays

`malloc` is called once for `9` times the table size plus the rectangle array.
The pointers are then carved out of it: `X=P=O=t+u` puts the rectangles right
after the table and `d=O=P+n` puts the pixels right after the rectangles. The
check `(L=(size_t)K)-K` is not zero only if the size does not fit in a
`size_t`. `simple.c` names the pieces `sums`, `rects` and `pixels`.

7. Comma operator and assignments inside expressions

Many statements are joined with commas and many values are assigned inside
other expressions. For example `U=X->d>U?(N=X)->d:U` remembers the worst
rectangle in `N` while it updates the biggest error `U`, and
`o.y+=N->h=l` sets the old rectangle's height and moves the new one down in
one go. `jpeg_set_quality(I,100,D.optimize_coding=1)` even sets a field while
passing an argument. `simple.c` uses separate statements and `if` blocks.

8. `?:` instead of `if` and `abs`

`u>e?e=u,k=z:0` is an `if` that keeps the best score, `e<f?...:(...)` chooses
between a horizontal and vertical cut, and `u+=L<0?-L:L` is an absolute value.
The rectangle count is clamped with `n<1?1:n>w*h?w*h:n`.

9. `while` loop used as an `if` with error exits

The whole program is inside `while(c==4){...}`. It never loops: the end of the
body returns 0, and every `break` (file did not open, out of memory) jumps
out to the `return 1` after it. `simple.c` checks `argc` and returns 1 directly.

10. Short, reused variable names

Every variable is one letter and many are reused for different jobs. `K` and
`L` are first the memory size, then parts of the cut score. `l` is both an
area and the best horizontal cut. `O` is a pointer to the scratch memory and
later the row pointer passed to libjpeg. Inside the `A` and `B` macros the names
`x`, `y`, `w` and `h` are declared again so they hide the outer ones.
`simple.c` gives each value its own descriptive name.
