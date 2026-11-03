# 1998/bas1

Difficulty: Hard

1. Shaped source code

The middle of the program is shaped like a cut-off triangle and the end of the
program narrows to a point. Tokens such as `main` and `(!O` are split over
many short lines so the shape stays. The shape has no effect on how the code
runs.

2. Single-letter macros that hide keywords and expressions

`#define p if(`, `#define z for (`, `#define e putchar(` and
`#define u U(533);` hide C keywords and calls. `#define a >> B &M[i X]` and
`#define X [ I+Z*!! M] [ N+Z*(M!= 2)]` are partial expressions that only make
sense once pasted after other tokens, so `32 a` really means
`(32 >> B) & i[I + Z*!!M][N + Z*(M != 2)][M]`. `g` and `n` build on `a` to test
whether a beam goes in direction `B` or in the previous direction. Running the
preprocessor and replacing each use with a named helper (`HAS(d)`, `emit()`,
`shade()`) undoes this.

3. Macro with the same name as a variable

`O` is the parameter of `main` and also a function-like macro `O(Q)`. The
preprocessor only expands `O` when it is followed by `(`, so `O(10)` emits a
PostScript command while a plain `O` is the variable. The macro body starts with
`if(!O)`, so the code also sets `O = !n;` and `O = 0;` just to turn the next
macro call on or off, using the variable as a hidden `if` condition.

4. Recursive main with two passes

`main` calls itself at the end with `return O && main(!O);`. The first call
(with the argument count) builds the maze and prints the start of the gzip
file. The second call (with 0) skips the maze building because every building
step is guarded by `O`, and instead draws the maze and writes the gzip trailer.
The same three nested `for` loops are used for both jobs. simple.c splits these
into `make_candidates()`, `build_maze()`, `draw_maze()` and `finish()`.

5. CRC register reused as a random number generator

`U()` updates the CRC-32 checksum that gzip needs. During the first pass the
same variable `A`, seeded with `time()`, is used as the random number: `U(533)`
just mixes one more byte into it. In the second pass `A` is reset to the CRC of
the already compressed PostScript prolog (`953351751`) so the final checksum is
correct.

6. Bits of a number used as a hidden byte

`U(O)` halves its argument and feeds the low bit into the CRC until only the
value 1 is left, so the top set bit marks the end of the data. `642 + Q` halved
is `256 + 'A' + Q/2`, so `U(642+Q)` adds the letter `'A' + Q/2` to the checksum
and `U(533)` (`533 / 2 = 256 + 10`) adds a newline. Each call also adds one to
`E`, the uncompressed length. The CRC step constants `57525273` and `9583591`
add up to `2^26`, so `(57525273 + bit*9583591) << 5` is either `0x6DB88320` or
`0x80000000`; this is the normal CRC-32 polynomial `0xEDB88320` applied to an
inverted register, so no final inversion is needed.

7. Hand-made deflate stream

The program does not call a compression library. The gzip header and a
deflate block containing the PostScript prolog are stored ready-made in the
string `r`. The block's Huffman table was chosen so that every following line
of output, one letter and a newline, compresses to exactly one byte. Those
bytes are `r[0]` to `r[16]` with the high bits masked off (`& 63`), so
`putchar(r[Q/2] & 63)` writes a whole compressed PostScript line. The bytes `0`
(four newlines) and `1` (two newlines and end of block) finish the stream.

8. Encoded string with escape characters

The bytes of the gzip header and prolog in `r[17]` onwards are written using
Latin-1 characters. A space works as an escape: the byte after it has 32
subtracted, which lets the string hold control characters without writing
`\` escapes. The printing loop `N - 32 | !O ? N : r[++I] - N` does the decoding.
simple.c keeps the decoded bytes as a plain hex array.

9. Avoiding newline bytes in binary output

At the end the CRC and length are written into `r[0..7]`. If any of these is a
newline (10) or carriage return (13), a byte holding four more newlines is
written, the checksum and length change, and the check starts again. The index
`(I > 3 ? E : A) >> (I*9 & 24)` uses `I*9 & 24` to get the shifts 0, 8, 16 and
24 for little endian bytes.

10. Side effect hidden in an unrelated statement

Every call to the `O(Q)` macro runs `*i[25][41] |= U(...)`. `U()` always
returns 16, so this marks a beam start in direction 1 at the far corner cell,
which is the maze exit. This happens long before that cell is drawn. simple.c
sets the exit once at the start of `draw_maze()`.

11. Packed beam records stored at random positions

Every possible beam is packed into one number,
`N<<7 | Z<<13 | I*4+M` (column, length, row and direction), and stored in a
random empty slot of the 32768 entry array `G`. Reading `G` from start to end
then visits the beams in random order. Empty slots decode to a beam of length 0
from cell (0,0) to itself, which is skipped. The command line argument count is
the shortest beam length, which is why more arguments change the maze.

12. Union-find with raw pointers

`F` is an array of `void *`. `Q()` follows the pointers to the root of a
connected group and shortens the path on the way back
(`*O = Q(*O)`). A beam is added only when its two ends are in different groups,
which is Kruskal's algorithm for building a maze. simple.c names these
`parent` and `find()`.

13. Bit tricks for cell contents

Each cell holds six bits, one per direction around a hexagon. `32>>M` is a
beam starting, `4>>M` a beam ending and `36>>M` a beam passing through.
`65>>B` and `1040>>B` select the neighbouring directions `B-1` and `B+1`
without a `% 6`. The test `!(v%9 | v/9 & v/9-1)` checks that a cell is empty
or has a single straight beam passing through (0, 9, 18 or 36) so a new beam
may cross it.

14. Array index written backwards

`B[G]`, `E++[G]`, `M[i X]`, `N[I[i]][1]` and `I[r]` use the rule that `a[b]`
is the same as `b[a]` in C to make the array names hard to spot.

15. Comments placed inside include lines

`#include/* s */<stdio.h>` puts a comment between `#include` and the header
name. It still works, but simple text searches for include lines miss it.
