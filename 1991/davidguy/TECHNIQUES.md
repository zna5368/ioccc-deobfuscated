# 1991/davidguy

Difficulty: Insane

1. Macros that hide keywords and pieces of statements

`#define W while(` hides a keyword and an opening parenthesis, so the code
reads `W L<n){...}`. `#define r register u` and `#define w char*` hide types.
`P` and `C` are worse: `P` is `if(i^z){x=Q+(n=` and `D(q,s)` starts with
`c);`, so `P}` only becomes a complete `if` statement after both macros are
expanded and joined together. Each one reverses the board in memory (by
words or by bytes) but nothing in the source tells you that.

2. A "pointer that walks and casts" macro for building binary messages

`#define j(x)(*(*(x*)&T)++)` treats the `char *T` as if it were a pointer
of type `x`, reads or writes through it and moves it forward by the size of
that type. `H`, `K` and `e` use it for 16-bit, 8-bit and 32-bit values.
A line like `K=55;K;H=6;e=F;e=I;` is really "byte 55 (CreateGC), skip a
byte, 16-bit length 6, 32-bit id, 32-bit window". The same macros used as
plain statements (`e;` or `K;`) skip over fields of the server's reply, so
the meaning of each value is only known by counting bytes against the X11
protocol specification. In `simple.c` these became `put8`, `put16`,
`put32`, `get8`, `get16`, `get32` and `skip` with a comment naming each
field.

3. Talking to the X server with raw protocol numbers

There is no Xlib and no header files, so every request is a list of magic
numbers: 55 is CreateGC, 53 CreatePixmap, 2 ChangeWindowAttributes, 72
PutImage, 63 CopyPlane, 61 ClearArea, 12 is the "foreground + background"
value mask, and 8192 is just a gap between the resource ids the program
makes up. Even the port 6000 is written as the two bytes `K=23;K=112;`.

4. Clever constants

`H=17004` writes the two bytes `'l','B'` (0x426C). The X server reads the
first byte to learn the client's byte order, and because the short is stored
in the machine's own order the right letter lands first automatically. Right
after, `i=66==*s` checks whether that first byte was `'B'` (66) to find out
if the machine is big-endian. The Unix socket path is stored as
`".slo.-W00,tmhw.W/"` with every character one less than the real
`"/tmp/.X11-unix/X0"`, and `1+*s++` decodes it while copying.

5. Reusing values in surprising ways

`argc` (`a`) is used directly as the socket address family (1 is AF_UNIX,
2 is AF_INET) and to compute the address length `24-a*4`. The connect loop
tries `socket(a, 3-Q++, 0)` so it keeps trying socket types 3, 2, 1, ...
until one works, which covers systems where SOCK_STREAM has different
numbers. The digit test `(F=*s++)&16` uses the fact that the digits
`'0'`-`'9'` have bit 16 set while `'.'` and the string end do not.

6. One-letter variables reused for different jobs

`Q` is first the socket-type counter and later the number of words in the
board. `U` is first the number of pixmap formats and later the board width
in words. `B` and `O` hold the black and white pixel values and are then
overwritten with the ids of the bitmap and its GC. `f` is the maximum
request length and then the number of rows per PutImage. In the Life
function `T`, `v`, `X`, `L` and `A` mean completely different things from
the globals or `main` variables with the same names.

7. One shared buffer with leftover bytes

The global array `X` holds the socket address, the connection request, the
server's reply and every request sent afterwards. Pad bytes that are only
skipped (`K;`) keep whatever data the reply left there, and they are sent
to the server like that. `simple.c` keeps this behaviour with `skip()` so
the bytes sent are identical.

8. Bit-sliced Life with shift registers

The Life step in `v()` works on 32 cells at once, one per bit. The macro
`E(a,b,c)` shifts a word right by one bit and pulls the missing top bit out
of `c`, a "history" word that collects the lowest bit of every row it has
seen. Because the loop walks down a column of 32 rows, the bit it pulls out
was saved exactly 32 steps ago: from the same row but the word to the left.
`V` loads a row and builds the three neighbour cells with two such shifts,
`Z` adds two cells to a running count kept as separate bit-planes (`I` for
ones, `c` for twos, `M` for "4 or more"), and the final expression
`c&~M&(I^d|N)` means "count is 3, or count is 2 and the cell is alive".
Since the computed cells come out one pixel off, the last loop of `v()`
shifts every row back by one bit. In `simple.c` these became `shift_in`,
`load_row`, `add_pair` and named counter fields.

9. Hidden byte and bit order conversion

`i` (client vs. server byte order) and `z` (server bit order) decide whether
the board is reversed word by word and/or byte by byte before it is sent,
then reversed again afterwards. Reversing all words turns the picture
upside down and mirrored, which is fine for Life and fixes the bit order
for servers that want the lowest bit first.

10. K&R style and implicit declarations

`main(a,b)w*b;`, `L(A)r*A;` and `v(n,O)u*n,*O;` use old-style parameter
declarations, all functions return an implicit `int`, and `socket`,
`connect`, `sbrk` and friends are used without any header (the Makefile
adds them with `-include`). Variables are declared `register` everywhere.
