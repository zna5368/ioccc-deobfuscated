# 2006/stewart

Difficulty: Medium

1. A single `void *` holding every variable

Apart from `argc` and `argv`, the only variable is `void* v=malloc(0170UL)`,
which is 120 bytes, or 15 slots of 8 bytes. Each "variable" is a slot number
inside that block: slot 2 is the image size, slot 1 the iteration count,
slot 0 the bitmap size in bytes, slot 6 the bitmap pointer, slot 8 the IFS
table, slots 9 and 10 the current point `x` and `y`, slot 13 the `FILE*`,
slot 14 the random seed, and so on. Some slots hold a `long`, some hold a
`double`, and some hold a pointer stored as a `long`. Since the slots only
have numbers, nothing tells you what each one means. The rewrite uses normal
local variables with names.

2. Casting macros that hide the slot accesses

`Y(x,y)` is `(double*)x+y`, the address of slot `y`. `Z(x,y)` is
`*(long*)Y(x,y)`, which reads slot `y` as a `long`. `F` and `U` read a slot
and cast it to `FILE*` or `unsigned char*`. These macros nest, so
`*Y(Z(v,8),3)` means "read the pointer in slot 8 of `v`, then read the
`double` in slot 3 of the block it points to", which is `ifs->x_max`. After
preprocessing there are hundreds of casts. The rewrite uses a
`struct ifs` and a `struct transform` so each field has a name.

3. A table laid out by hand

The IFS table is `malloc(050+Z(v,3)*070)`, which is 40 bytes plus 56 bytes for
each map. The first 5 slots are the count and the x/y limits. After that come
7 doubles for each map. The macro `X(x,y,z)` is `(double*)x+y+7*z`, so
`X(Z(v,8),9,k)` is the field at offset 9 of map `k`, which is `e`.
Offsets 5 to 11 are `a b c d e f p`. The rewrite uses the same layout as a
struct with a flexible array member `maps[]`.

4. Reusing slots for different jobs

Slot 3 first holds the number of maps read from the file. It is then copied
into the table and used as the loop counter for reading the file, for the
main iteration loop and for the output loop. Slot 4 is first the "previous
seed" that the random number macro writes into, and right after that it is
the loop counter for picking a map. Slots 11 and 12 first hold the new point
and later the point scaled to the range 0 to 1.

5. A misleading if statement

```c
if((*Y(v,9)>=...))
   *Y(v,11)=...,
*Y(v,12)=...,
*(U(v,6)+...)|=...;
```

The indentation makes it look like only the first line belongs to the `if`
and the next two lines always run. In fact the lines end with commas, not
semicolons, so all three are one expression joined by the comma operator, and
all of it is inside the `if`. The rewrite uses braces and separate
statements.

6. A home made random number generator

Instead of `rand()`, `R(x,y)` is an xorshift generator:
`y^=y<<11, y^=y>>15, y^=y<<3`. It also saves the old seed in `x` and returns
`(old+new)&0x7fffffff`, which is a number from 0 to 2147483647. The starting
seed `8675309` is a famous phone number from a song. The rewrite puts it in
`next_random()`.

7. Number literals in strange bases and shifts instead of division

The sizes are written in octal: `0170` is 120, `050` is 40 and `070` is 56.
`Z(v,2)*Z(v,2)>>3` divides the number of pixels by 8 to get the number of
bytes, because each byte holds 8 pixels. The same pixel index is then split
with `>>3` (which byte) and `&7` (which bit inside the byte). The rewrite
uses `/ 8`, `% 8` and `sizeof`.

8. Repeated pixel math inside one long expression

The pixel position
`(((long)(Z(v,2)-*Y(v,12)*Z(v,2))*Z(v,2))+(long)(*Y(v,11)*Z(v,2)))` is
written out twice, once to find the byte and once to find the bit. It works
out `row*size + column`, where the row is flipped (`size - y*size`) because
in a picture y grows downwards. The rewrite works out `row`, `col` and
`pixel` once each.

9. Output that is C code

XBM images are really C source code. The single `printf` writes
`#define x_width`, `#define x_height` and `static char x_bits[] = {` and
then each byte as `0x%02x`, which makes the output look like a program
instead of a picture.

10. Short macro names

`P` is `printf`. With names that short, and with `printf("bad args\n"),exit(1)`
joined by a comma, it is easy to miss what a line does.
