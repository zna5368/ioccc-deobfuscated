# 1994/horton

Difficulty: Easy

1. Code shaped into a picture

The source is laid out with tabs and line breaks so that the code itself
draws the shape of a cubic curve. Single tokens such as `r`, `,`, `i`, `(` and
`]` sit alone at the start or end of a line, and the declaration
`int r, c, bi, bj, i;` is stretched over six lines. This makes the program hard
to read top to bottom, as the README notes it "confuses cb greatly".

2. Command line macros hide part of the code

The Makefile passes `-DE='exit(1)' -DA=argc`, so the first statement
`if(A<5)E;` is really `if (argc < 5) exit(1);`. Without looking at the
Makefile you cannot tell what `A` and `E` are.

3. A macro that hides an assignment

`#define S(r, c) f[r][c] = 1;` sets one pixel of the bitmap. The macro already
contains a semicolon, so calls like `S(i, 18);` produce an extra empty `;`
statement, and the call at the end of the `if` hides the fact that only one
assignment is controlled by that `if`.

4. Magic numbers with the meaning removed

All the plotting constants are written as bare numbers. `10`, `80`, `20` and
`150` are the edges of the plot area, `18` and `152` are the box edges two
pixels outside it, `-20` and `10` are the x range, and `500` is the top y
value. The scale factors `.07` and `4.33333` are the pre-computed values of
`70 / 1000` (pixel rows per y unit) and `130 / 30` (pixel columns per x unit).
The coefficients are also re-read with `atof(argv[n])` every time the loop
runs instead of being stored in named variables.

5. A two dimensional table flattened into one string

The character for each screen cell comes from a 16x16 table (the "graphtab"),
which is stored as one long 256 character string full of escaped backslashes
and quotes. The index is computed as `left * 16 + right`, where `left` and
`right` are the four pixels of the left and right column of a 4x2 block packed
into a 4 bit number with `<<3`, `<<2`, `<<1` and `<<0`. The string literal is
indexed directly, as in `"..."[i]`.

6. Newline hidden at the start of the table

The table string begins with an extra `"\n"`, so the computed index has `+1`
added to skip it. The whole index is then multiplied by `(c<79)`. On the last
column of a row the comparison is `0`, the index becomes `0` and `putchar`
prints the newline from the start of the string. So one `putchar` call prints
both the picture characters and the end of each line, with no `if`.

7. Meaningless names and reused variables

The bitmap is called `f`, and the variables `r` and `c` are first the pixel
row and column of a plotted point and later the screen row and column of the
output loop. The variable `i` is first a loop counter for drawing the box and
later the index into the character table.
