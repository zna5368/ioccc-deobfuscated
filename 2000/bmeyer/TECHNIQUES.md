# 2000/bmeyer

Difficulty: Terrifying

1. Single-letter names reused for many jobs

Every global is one letter (`G,l,I,C,B,A,W,L,S,R,O,c,k,s`) and the meaning
changes as the program runs. `C` is first the PNM magic number, then the
output mode, and inside the row loop a local `C` hides it and holds the
prediction and later the prediction error. `K` is the terminal width and later
the pixel read from the input. `J` is a matrix pointer in one place, a buffer
size, and then the row counter. `l` is the image height, a string pointer and
a local `double`. simple.c gives each job its own name (`width`, `height`,
`print_mode`, `pred`, `columns`, `value`, `row`...).

2. Text-saving macros

`#define M M(`, `#define X +=`, `#define F for(`, `#define D return`,
`#define V =malloc(sizeof(` and `#define i =m(p(r,o,v),e,d` hide brackets and
whole expressions. `C i)+.5` is really `C = m(p(r,o,v),e,d) + .5`, and
`y i/.9)` is a second call with a different last argument. A function-like
macro `Z(x)` is even defined in the middle of an expression. These are all
expanded in simple.c.

3. One function that does everything (`M`)

`M(R)` first prints any value waiting in `s`, then depending on the sign of
`R` it reads a number or byte from stdin (`R > 0`), computes the geometric sum
`.9 + .7 * M(R+1)` recursively (`R < 0`) or just returns 0. Even the character
printed after a decimal number (space or newline) depends on `R & 7`. simple.c
splits it into `flush_output()`, `read_input()` and `weight_sum()`.

4. Output buffered through printf's return value

Instead of a flag, a value is "waiting to be printed" when `s >= k`. After
printing, `s` becomes the return value of `printf()` (the number of characters
written), which is always smaller than `k = 256`. The encoder then shifts bits
into `s` with `s += x + s`; the 1 that `printf()` returned acts as a marker
bit, so `s` reaches 256 exactly when 8 new bits have been collected. During the
header `k` is 0, so every assignment to `s` is printed by the next `M()` call.
The format is chosen by pointer arithmetic on a string: `"%ld%c" + 3*(C<1)`
is `"%c"` for binary output.

5. Hidden arithmetic coder (`_`)

`_(J,x)` is a bit-by-bit arithmetic coder. `W` and `A` are the low and high
ends of the interval, `L` the decoder's code value, `S` the count of postponed
bits. The three renormalisation cases (upper half, lower half, middle/
underflow) are squeezed into one `for` condition that also computes how much
to subtract (`J *= x + O`), and the postponed bits are written by flipping
`O = x = !J` in the loop increment. The same loop serves encoder and decoder;
`S-- * B` silently disables the bit output when decoding (`B == 0`). simple.c
writes it as `code_bit()` with an explicit `if` for each case.

6. Hidden linear algebra (`m` and `p`)

`m()` solves a 12x12 weighted least squares system with Gaussian elimination
using only pointer walking (`W += 14` moves down the diagonal, `A -= U` moves
up a column). The current neighbours are stored as minus the last row, so the
bottom right element after elimination is directly the predicted value. It
also adds a "ridge" bias that pulls the answer towards averaging the
neighbours. `p()` computes `O = J + 0.8*x` on whole 13x13 matrices and is
used both to build the weighted sums and, with a zero matrix, to clear or
scale matrices. simple.c names them `predict()` and `blend()`.

7. One big memory block carved into many arrays

A single `malloc` of `double`s holds the squared-error sums `h`, their row
sums `q`, the neighbour vector `e`, a row of column matrices, a row of
horizontal sum matrices and three scratch matrices (`r`, `v` and a block that
is never written and therefore always zero). Code relies on reading those
never-written zero blocks (`p(H,H,r)` zeroes `r`). The `long` image buffer
also uses its first row as the progress-display accumulator. simple.c
allocates separate, named arrays.

8. A string with two meanings

`" !{   ,;lf6D@"` is both the list of characters for the ASCII-art progress
picture and, through `*l%3` and `*l%5`, a coded list of the moves from one
neighbour pixel to the next. Afterwards `l[...-8]` reaches backwards from the
end of the string to pick a shade. simple.c uses a table of
`{dx, dy}` neighbour offsets and a separate `shades` string.

9. Uninitialised-looking loop variables

Many loops start with `for(;` because their variable already happens to have
the right value: `x` is 0 after clearing `h` and after each row, `J` is -1
after filling the image buffer so `++J < l` counts rows from 0, and `H` points
just past the last column matrix after the first loop of a row. simple.c
initialises every loop counter.

10. Pixels processed with a backwards counter

`x` counts from the width down to 0 while the image pointer `j` moves
forwards, so `x` is the distance from the right edge. All the "left" sums
are therefore built with `x` increasing. simple.c keeps `x` but documents it.

11. Recursion instead of loops

`t()` evaluates the Student t distribution with a recursive series that ends
when `x > 13` (and still adds `1/sqrt(K)` in that last step), and `M()` with a
negative argument sums a geometric series recursively. simple.c uses loops
that do the same floating point operations in the same order.

12. Encoder and decoder hidden in the same code

Compressing and decompressing share every line. The difference is carried by
`B` (0 when decoding, -1/-2 when encoding raw/ASCII input) and by the magic
number being stored as `15 - n`. For example `M(-B)` reads a pixel only when
encoding, `S%8==B` reads a compressed byte only when decoding and
`s = B ? s : Q+k` prints a pixel only when decoding. simple.c uses an explicit
`encoding` flag.

13. Misused scanf and assignments as conditions

`scanf(" %[#]%*[^\n]", &O)` stores the `#` of a comment into a `long`,
and the return value of `scanf()` is used as the loop condition to skip
comment lines. `!putc(...)` is used to reset an accumulator to 0 because
`putc` returns a non-zero character.
