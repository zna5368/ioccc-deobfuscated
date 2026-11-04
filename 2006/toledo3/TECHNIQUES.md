# 2006/toledo3

Difficulty: Terrifying

1. Source shaped like a picture

The code is laid out as a big "V" in the upper half and a checkerboard of
10 character blocks in the lower half. Statements, numbers and even tokens
like `1e7` are cut wherever the shape needs a line break, so the layout tells
you nothing about the structure. Running the code through a formatter undoes
this.

2. Macros that hide keywords and repeated code

`#define H while(`, `#define D ;if(`, `#define P return y=~y,` and
`#define a X(0,0,0,` make the code look like a stream of capital letters.
`v` is the whole "redraw the board" loop and is used four times. `_(a)`
writes one byte of a bitmap and is used eight times in a row. `R(a)` builds
the castling bits one square at a time. `G(p)` uses token pasting (`p##Pixel`)
so that `G(Black)` becomes `BlackPixel(...)`.

3. One letter names, implicit `int` and K&R style

Every variable is a single letter, and the same letter means different
things in different places: `d` is the X display in `main` but the move
distance in `X`, `C` is a graphics context in `main` but a direction index
in `X`. The search function `X(w,c,h,e,S,s)` and the globals `y,u;` have no
types at all, so old C makes them `int`. Global `i`, `z` and `b` are both the
loop counters of the redraw macro and the "move to play" (piece, from square,
to square).

4. One string used as many tables

The string `l` holds the back rank pieces (`"dbcefcbd"`, using `7 & 'd'` = 4
for rook and so on), the centre bonus (`"abcddcba"`, only differences are
used), the piece values (`"~WAB+"` read as `char - ' '`), the start square for
the search (`'d'` = 100) and all move directions (`"+-84HLSU?A6J57IKJT576,"`
read as `char - 64`). After the board is set up `l` has been moved 8 places
forward, so the search reads everything at different offsets than the setup
did. Two more strings, `"51#/+++"` and `"95+3/33"`, are indexed by piece type
to give the first and last direction in `l` for each piece.

5. Pictures stored as text

The piece images are in the string `F`, one character per row of a piece.
Each bit (64, 16, 8, 4, 2, 1) of the character says whether a 8x8 pixel block
is filled. Bit 32 is never used, it is only there so every character is
printable (`' '`, `'>'`, `'~'`). The pointer `F` walks backwards and forwards
through the string (`F -= z%7-6 ? z%14<6 ? 16 : 0 : 8`) to produce all 56
bitmaps (4 square shades times 14 board values) in the order the drawing code
wants them. The square backgrounds come from `"U\252U\0DDDD"`, bit patterns
that look grey on a black and white screen.

6. Colour by complement

White pieces are stored as `~type` and black pieces as `type`, and the side
to move `y` is `0` or `~0`. So `piece ^ y` is positive for your own pieces
and below `-1` for enemy pieces, and `y|1` is `+1` or `-1` to step through
piece codes or pick the pawn direction. The search flips `y` on entry and
every `return` flips it back, except the one that plays a move, which is how
the turn passes to the other player.

7. Reversed array indexing

`80[m]`, `b[I]`, `z[I]`, `O[I]`, `p[I]`, `q["51#/+++"]` and `C[l]` are written
the "wrong" way round. `a[b]` is `*(a + b)`, so `b[a]` is the same thing.

8. One function doing everything

`X` is the move generator, the evaluation, the alpha-beta search, the
quiescence search (`w` limits it to recaptures on one square), the check
detection (`X(0,0,0,63,1,0)`), the move picker (stores the best move in `i`,
`z`, `b` when `S` is 1) and the move player (plays the move in `i`, `z`, `b`
when `S` is 0 and it is legal). The same parameter `S` is both that mode
switch at the top and the alpha-beta bound below it.

9. Loops and branches hidden in expressions

Promotion, normal moves and castling all share one `while (n - t)` loop:
for promotion `n` runs through knight to queen, for castling the loop body
is run a second time by doing `d++` instead of moving `n` on. Castling itself
is a single expression `q>5&d<2&C+6<A&&(g=..., r=..., m=..., !(...)&&...)
? d++ : (n+=y|1)` that also moves `p` two squares and finds the rook. The
pawn double step is skipped by adding a boolean to the loop counter
(`C += q<2&C+3>A&(...)`), and sliding pieces keep going because the
`do`/`while` condition only resets `p` when the square was not empty.

10. Reused variables and pointers

The board array `I` is first used as a 512 byte buffer for building the
bitmaps through the `char *f` pointer, then wiped and filled with the board.
`g` points to the pawn taken en passant or, for castling, the square the rook
lands on. `m` is the rook. The castling and en passant flags are packed in
one integer: bits 0..5 for kings and rooks that have moved (built with the
`R(a)` macro from the corner squares 21, 28, 91, 98) and the en passant
square shifted left by 6.

11. Magic numbers for X11 constants

`XSelectInput(d,w,32772)` is `ExposureMask | ButtonPressMask`,
`XCreateGC(d,w,4,&g)` is `GCForeground`, and `e.type==12` and `e.type==4` are
`Expose` and `ButtonPress`. `*W[1]-45` turns the level `'1'..'3'` into a
search depth of 4..6, and `-1e8`, `1e6` and `1e7-811*h` are "no move yet",
"a king was lost" and "king captured, sooner is better".
