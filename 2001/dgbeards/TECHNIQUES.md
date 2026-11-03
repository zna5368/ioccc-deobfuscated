# 2001/dgbeards

Difficulty: Hard

1. Macro compression

Most of the code is hidden behind macros. `C(j,k)` opens a `for` loop (its
closing brace is written later by hand), `A(x,y)` and `B(x,y)` append a move to
the quiet-move list `i[]` or the capture list `j[]`, `D(x)` generates all
knight or king jumps from an offset table, and `O(v)` slides a bishop, rook or
queen in one direction. The Makefile also defines `E=break`, `F=char`,
`K=case`, `P=int`, `R=return` and `I=0xFFF` on the command line, so even the
keywords are not in the source. The code only makes sense once it has gone
through the preprocessor.

2. Strings encoded as base 4 with four symbols

Every printed string and the starting board are written using only the
characters `{`, `}`, `;` and space, each followed by a filler character. The
function `G()` turns the table `S[]="{}; "` into the lookup table `W[]`
(`{`=0, `}`=1, `;`=2, space=3), then reads four symbols at a time and joins
them into one byte (`d=d*4+W[...]`). For example the string passed to `G()` in
`main` decodes to `"Value %d\n"` and the one in the illegal move branch decodes
to `"Illegal move\n"`. `X(x)` is `G(x,L)`, which decodes into the scratch
buffer `L`, the same buffer that is also used for reading user input.

3. Run-length encoded board inside an encoded string

After `G(v,v)` decodes `v` in place, `s()` unpacks it into the board `p[]`.
Each byte minus 120 is a square's value, and a `^` starts a run of the form
`^`, count + 32, value + 120. This turns the 2-row-per-side starting position
plus empty space into a short string. In `simple.c` the board is set up from
the string `"RNBQKBNR"` instead.

4. Pieces as ASCII codes and signs as colours

Pieces are stored as their letter codes, but the code writes them as plain
numbers such as `K 80:` (`'P'`), `K 78:` (`'N'`), `81`, `66`, `82` and `75`.
White pieces are positive and black ones negative, so `p[a]*p[b]<0` means
"enemy piece" and `p[c]*t<0` means "belongs to the side not to move".

5. XOR-hidden promotion pieces

The promotion pieces come from `g[]="KSLAU"` XORed with the loop index:
`g[_]^_` gives `K`, `R`, `N`, `B`, `Q`. Nothing in the source says which
pieces a pawn can become.

6. 0x88 board arithmetic

The board is a 16-column array where only columns 0 to 7 are real squares.
Tests like `!(c+x&136)` (136 is `0x88`) check whether a square is on the
board, `c&112` extracts the row, and magic offsets such as `-17*t`, `-15*t`
and `-32*t` are the pawn's capture and double-step directions. The input
parser uses `896-16*L[1]` (which is `16*('8'-L[1])`) to turn a rank character
into a row.

7. Reused global variables with short names

Single-letter globals such as `a`, `b`, `c`, `d`, `e` and `_` do different jobs
in different places. `a` and `b` are move counters in `f()`, string indexes in
`G()` and loop counters in `main`. `e` is a column counter in `f()`, a score in
`V()` and finally the index of the best move. Because `e` is shared, an early
return from `V()` leaves the best move index at 8 (the value `f()` left
behind). Local variables also shadow functions and globals: inside `V()` the
names `s`, `G` and `u` are locals that hide the functions `s()` and `G()` and
the global move `u`, and inside `w()` the local `v` hides the board string.

8. Single-letter function names

`f()` generates moves, `G()` decodes strings, `m()` makes a move, `h()` prints
the board, `s()` sets up the board, `w()` evaluates a position and `V()` is the
search. Their names give no hint of their purpose.

9. Short circuit evaluation and the ternary operator instead of `if`

`c%8==0&&putchar(56-c/16)` prints the rank number,
`p[_]&&(p[_]*t>0&&v--,p[_]*t<0&&v++)` counts material, and
`H<5?H==1?n++:(o<6?o++:0):0` decides search extensions with nested ternaries.
`c%8==7?J X(...),c+=8:0` both prints the row separator and skips the 8
off-board squares in one expression, and `a==b?J X(...):(m(u),t=-t)` chooses
between rejecting and playing the move.

10. Packed assignment in one expression

`s=(e=-V(n-1,o))>s?Y=G,e:s;` searches a move, stores its score, compares it,
and records the best index all at once. The strict `>` here means the best
index `Y` is never set when every move loses, which is the intentional bug
that makes the program crash when it loses.

11. Fall-through in the piece `switch`

The queen case `K 81:` has no body and falls into the bishop code, which then
checks `p[c]==66||p[c]==-66` to decide whether to stop or fall further into the
rook code. One block of code therefore serves bishops, rooks and queens.

12. time() as a random number generator

`time(NULL)%9-4` is added to every leaf evaluation to make the play vary
slightly. Because the time changes only once per second, it works as a slow and
unusual source of randomness without needing `rand()` and `srand()`.
