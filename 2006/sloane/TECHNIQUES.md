# 2006/sloane

Difficulty: Medium

1. Source shaped as ASCII art

The whole program is laid out to look like the donut and checkerboard it
draws. Tokens are split across lines, strings are broken into many small
adjacent pieces (`"E" "?yYr..." "dOVsJ"`), and a backslash-newline is used in
the middle of a string literal so the text fits the picture. The compiler joins
adjacent strings and ignores the whitespace, so none of the layout matters to
the program.

2. Implicit `int` and K&R style declarations

`_,x,y,o,N;` declares five global variables with no type at all, so old C rules
make them `int`. The functions `p(n,c)`, `c(q,l,r)`, `E(a)` and `main(k,Z)` also
have no return type, and their parameters are declared in the old K&R style
(`char*l,*r;` after the parameter list) or not at all, so they are `int` too.
`main` even reuses its `argc` parameter `k` as a loop counter.

3. Reused global variables with one-letter names

The globals `x`, `y`, `o` and `N` mean different things in different places.
In the banner code `x` is a column, `y` is a row offset (a multiple of 80) and
`o` is the left edge. In the checkerboard loop `y` is the row below the horizon
and `o`/`N` are floor coordinates. In the donut loop `x`/`y` are the screen
position, `o` is the buffer index and `N` is the brightness. The local floats
`e`, `n`, `g`, `m`, `c`, `d`, `f`, `h`, `l`, `t` and `D` hold the sines and
cosines of the angles and other parts of the 3D maths.

4. Huffman coded, run-length compressed banner

The "IOCCC 2006" banner is not stored as text. It is a list of (character,
repeat count) pairs, and both the character and the count are stored as
Huffman codes in a single bit string. Each printable character of the long
string, minus 62, gives 6 bits, and `_` counts which bit is being read
(`[_/6]-62>>_++%6&1`). The function `c(q,l,r)` walks a binary tree: `l` and `r`
are strings holding the left and right child of each tree node, starting from
node `q`. Each bit picks `l[q]` or `r[q]`, and subtracting 99 gives the next
node. A negative result is a leaf, and its value is used as an index into
`" /\\\n~|_."` (the character) or into `"#$%&'()*+,-.1"` minus 34 (the repeat
count). The simple version stores the decoded banner directly as six strings.

5. Drawing with a chain of `?:` operators

The function `p(n,c)` draws a character `n` times using nested conditional
expressions instead of `if` statements:
`c==10?y+=80,x=o-1:x>=0?80>x?c!='~'?b[y+x]=c:0:0:0`. A newline moves down one
row (80 characters) and back to the left edge (`o-1` because the loop then adds
one). Otherwise the character is only stored if its column is on the screen and
it is not `~`, which is used as a transparent character so the checkerboard
shows through the banner.

6. Comma operator to cram work into one statement

Several loops have no braces because all of their work is joined by commas
into one expression, for example the checkerboard loop
`y=-k/80-10,o=41+...,N=A-100.0/y,b[k]=".#"[o+N&1],z[k]=0;` and the z-buffer
test `if(D>z[o])z[o]=D,b[o]=...`.

7. Hidden truncation from float to `int`

Many results are floating point values stored into the `int` globals, which
silently throws away the fraction. This is how the checkerboard gets its
squares: `o` and `N` become whole floor coordinates and `".#"[o+N&1]` picks a
character from whether their sum is odd or even. The donut position `x`, `y`
and brightness `N` are rounded the same way to land on whole character cells.

8. Indexing into string literals

Instead of lookup tables or `if` chains, characters are chosen by indexing
straight into a string literal, such as `".#"[...]` for the checkerboard and
`" ..,,-++=#$@"[N>0?N:0]` for the donut shading (darker characters for brighter
light).

9. Pre-multiplied 3D maths

The rotation, perspective and lighting of the donut are written out as a few
pre-multiplied formulas (for example `D=15/(c*h*e+f*g+5)` is one over the depth,
scaled) rather than with separate rotation steps, which makes them hard to
recognise. The checkerboard uses `1.3/y` and `100.0/y` so that rows further
below the horizon are closer and show bigger squares.

10. Escape codes and a hidden first column

`puts("\x1b" "[2J")` clears the terminal and `printf("\x1b[" "H")` moves the
cursor home before each frame. The output loop `putchar(k%80?b[k]:10)` starts
at 1 and prints a newline whenever `k` is a multiple of 80, so column 0 of the
buffer is never shown and each row is 79 characters wide.
