# 2012/endoh1

Difficulty: Hard

1. Source code shaped as its own input

The source is laid out as a picture: a tall column of code standing in a box
of `#` characters, with the preprocessor lines at the top. The program is meant
to be run on its own source (`./endoh1 < endoh1.c`), so every `#` (including
the ones that start `#include` and `#define`) becomes a wall and every other
character becomes water. Comments such as `//-'`, `/*  #` and `/** ##` are
used only to fill out the shape and finish the border.

2. Complex numbers used as 2D vectors with swapped axes

Every position, velocity and force is a `double complex`. The real part is the
row on the screen and the imaginary part is the column with its sign flipped.
This lets gravity be added with `p[3]=G` (a plain number points down) and moves
the reading cursor one column to the right with `w-I`. Turning a position into
a screen column is written `x=*p*I`, which multiplies by `I` to move the
imaginary part into the real part. In `simple.c` this is `-cimag(position)`.

3. Flat array instead of a structure

All particles live in one array `a[97687]` and every particle takes five
entries in a row: position, wall flag, density, force and velocity. The code
reaches them as `p[0]` to `p[4]` and steps with `p+=5`. Even the wall flag and
density are stored as complex numbers. `simple.c` uses a `struct particle`.

4. Assigning a complex value to an int to throw away parts of it

When a `double complex` is stored in an `int`, C keeps only the whole number
part of the real part. The code uses this on purpose: `x=w+2` rounds the cursor
down to the start of the next line, `y=*p/2` gets the screen row, and
`0<(x=1-w)` checks whether two particles are close enough to push on each
other. Assignments like `x=` look useless but they do this conversion.

5. Macros that hide the loops

The `#define` lines continue onto the following lines through `/* ... */`
comments that cross the line break, so a macro body is hidden among the
picture. `h` expands to `for(x=011;2012-1>x++;)b[x]`, a loop over the screen
grid, and the source writes `h=0` to clear it and ``h=" '`-.|..."[...]`` to fill
it with characters. `f(p,e)` is a loop over all
particles that runs the expression `e` in the loop test, and `z(e,i)` builds a
double loop over every pair of particles that resets `p[i]` to `e` and then
adds `w*` followed by whatever comes after the macro. The density and the force
code are both written with `z`, so the pressure and viscosity formula is split
between the macro and the text after it.

6. Work hidden in loop tests and the comma operator

The loop tests do the real work: `p[2]=p[1]*9,p<r` sets the starting density
before checking the loop end, and the particle update `*p+=p[4]+=p[3]/10*!p[1]`
is in the test of the drawing loop. The main loop prints the frame in its
increment part `puts(o),o=b+4`. Because these run once more when the test
fails, they also touch the empty slot just past the last particle, which does
no harm.

7. Nested conditional operators for the input parser

The whole input parser is one statement:
`w=x>10?32<x?4[*r++=w,r]=w+1,*r=r[5]=x==35,r+=9:0,w-I:(x=w+2)`. A printable
character makes two particles (one at `w` and one at `w+1`) and sets their wall
flags from `x==35` (`35` is `#`), any character moves the cursor with `w-I`,
and a newline moves to the next pair of rows with `x=w+2`.

8. Reversed and chained array indexes

`a[b]` is the same as `b[a]` in C, so the code writes `4[...,r]`, `x[b]` and
`1[1[*t|=8,t]|=4,t+=80]=1`. The last one sets the four corner bits around a
particle on the screen in a single expression, using the comma operator inside
the index to move `t` down one row in the middle. Note that the last corner is
set with `=` instead of `|=`, so it erases bits other particles already set.

9. Encoded string and number literals

The table of drawing characters is split into pieces
``" '`-.|//,\\" "|\\_" "\\/\x23\n"`` so it is hard to read, and `#` is written
as `\x23` so it is not mistaken for a wall. The screen buffer starts with
the escape sequences `"\x1b[2J" "\x1b" "[1;1H"`, and printing from `b` the
first time and from `b+4` after that skips the clear screen code. The grid
start index is written in octal (`011` is 9), and `x%80-9?...:16` picks the
newline at the end of each row.

10. Marching squares with bit flags

Each screen character stands for the corner between four particle cells. A
particle sets bit 8, 4, 2 and 1 in the four characters around it, and the
4-bit number picks a character from the table, for example `_` for a water
surface and `|` for a side. In `simple.c` the bits are named `BOTTOM_RIGHT`,
`BOTTOM_LEFT`, `TOP_RIGHT` and `TOP_LEFT`.
