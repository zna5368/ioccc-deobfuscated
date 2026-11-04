# 2006/sykes1

Difficulty: Hard

1. A C file that is also an HTML page

The file opens with `/* <body bgcolor = 0 > <img src= cube.gif><!-- */` and ends
with `/*--> */`. To the C compiler the first line is a comment. To a web
browser it is a `<body>` tag, an `<img>` tag that shows `cube.gif`, and the
start of an HTML comment `<!--` that hides all of the C code until `-->` at
the very end. The Makefile just copies `sykes1.c` to `sykes1.html`. The
comments are also filled with `; ; ;` so that the code makes the shape of a
cube. The rewrite is a plain C file.

2. Macros that hide brackets, loops and `if`

`#define _ ] [` makes `q[13 _ 13]` mean `q[13][13]` and `p[0 _ v _ j]` mean
`p[0][v][j]`. `#define W ]=` makes `O[c W-1` mean `O[c]=-1`. `L` is `for(`,
`J` is `if (`, `H` is `unsigned long long`, and `K(o,O)` is a complete loop
header `for(o=0; o<O ; o++)`. Some `#define` lines put a comment between
`#define` and the macro name (`#define /* ; ; */ L for(`), which hides the
name. The rewrite uses normal `for`, `if` and array syntax.

3. Single letter global variables reused for many jobs

Almost every variable is one global letter, and most of them are used for
several things. `c` is a piece shape while the pieces are rotated, a
placement counter during the search, and the building step number when the
solution is drawn. `C` is the solution counter, then "is this the newest
piece", then a loop counter. `d` is the search depth, but by the time the
picture is drawn it is always 12, so `T[d W`, `u==d` and `i/d` use it as
the number 12. `h` ends up as 20 and `a` as 48, and the GIF code depends on
those values. The rewrite gives each job its own named variable or constant.

4. A recursive `main()` with `goto` into a loop

The search has no helper functions. `main(0,0)` calls itself to go one
level deeper. `if(!d)` makes sure the piece setup only runs in the first
call. Inside the loop there are two labels: `r:` is jumped to by
`goto r` when a piece has only one possible place, which starts the check
again one level deeper, and `n:` is jumped to by `goto n` for a dead end. Both
gotos jump into the middle of a `for` loop body. The rewrite uses a
`search()` function with a `prune()` helper that returns whether to go
deeper, and an inner `for(;;)` loop for the "start over" case.

5. Bit tricks for 3D shapes

Each piece is a 27-bit number holding a 3x3x3 block, and the cube is a
64-bit number holding a 4x4x4 block. The macro `R(z)` moves each bit `j` to
position `z` to turn a piece. Two formulas are passed to it.
`Z = (j*3+j/9*3+2)%10+j/9*9` turns the piece around the vertical axis.
`j+(j-" /@"[j/9]+38)/3*6-6` turns it around the left-right axis, using a
string character as a number for each layer. To move a piece around in the
cube, the code shifts the 64-bit number by -37 to 58 places. It throws away
shifts that lose bits, or that let the shape wrap round into the next row or
layer. Those are the checks against `0x888888888888888ULL` and
`0xF000F000F000ULL`. The rewrite has named `rotate_vertical()`,
`rotate_horizontal()`, `mirror()` and `add_positions()` functions.

6. A macro that hides the end of a statement

`R(z)` ends with `c=h; ; ; ;`. So `J k==4)R(Z)` only puts the rotation
under the `if`. The `c=h` after it always runs, and on the first pass this
is what loads the piece shape into `c`. The rewrite says this directly
with `shape = ...`.

7. Mirror image from a negative number

With a negative argument, `k=h>>18^h&511; h^=k<<18|k;` uses the XOR swap
trick to swap the top and bottom layers of every piece, which turns each one
into its mirror image. At the same time `C=-C` is run once for each of the
13 pieces. 13 is odd, so the counter ends up positive. The rewrite calls
`mirror()` and uses `-wanted` as the counter.

8. Hidden numbers and text in shifted strings

The GIF file is written byte by byte from strings with an amount taken off
each character, using the macro `P(o,O,l)`, which writes `O[C]-l` for `o`
characters. `"qspbc\213t,J+ **"` minus 42 is `GIF89a` followed by the
width 586, height 288 and colour table flags. `"F$0sjyxhfujWSU(&%%%"` minus 37
is the `NETSCAPE2.0` looping block. `"K#.3 ***V****t,J+*1"` minus 42 is the
frame delay and image header. `"!\241 ["` minus 32 is the end of each frame,
and the last frame also gets the `;` at the end of the file. The colour
table is made from the string `D`, plus a brightness character from
`" H Zbjm "` for each group of 16 colours. The same string `D` also gives the
`fopen` mode: `D+11` starts with `"wb"`, and glibc ignores the junk after
it. The rewrite writes the bytes as numbers with comments, and has a table
of base colours and brightness values.

9. A hand-made uncompressed GIF

Each frame is 168840 bytes of pixels, written as 1340 blocks with 127
bytes each: the `" !"` minus 161 gives the block length 127 and the LZW clear code
128, then come 126 raw pixels. Sending a clear code that often stops the
LZW code size from ever growing, so the raw pixel bytes are valid codes and
no compression code is needed. The rewrite explains this in `write_gif()`.

10. Drawing done with packed strings and long `memset` addresses

The ASCII cube is one 42-character string
`"+----+/    /|+----+ ||    | +|    |/+----+"`. It is cut into six lines
using the offsets `"06=EMT"[C]-48` and the lengths `20-g[C]-g[5-C]`, which
come from the indent table `g`. Edges are removed by writing into numbered
positions such as `T[6]`, `T[21]` and `T[29]`. The GIF cubes are drawn by the
macro `M`, which builds one long pointer calculation
`E[c]+j*298+v*a+88-u*5+586*(...)` and then gets cut off in the middle by the
rest of the `memset` call. The rewrite keeps the six lines as separate
strings in `draw_text_cell()`. `draw_gif_cell()` works out the `x`, `y` and
face colours once and then fills the top, front and right faces.

11. Sentinel values hidden in the data

`v=*s` sets the "fewest placements so far" value to the first piece's shape,
186, and uses it as a large starting number. `if(k+1)` means "cell is not
empty", because empty cells hold -1. The rewrite uses a named variable
`fewest = 186` and `who != -1`.
