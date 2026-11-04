# 2005/toledo

Difficulty: Hard

1. One function for everything

`main()` calls `pain()`, and `pain()` does all the rest: it sets up the board,
prints it, reads moves, generates moves, scores them, searches the game tree,
checks for check and castles. It decides which job to do from the board
itself: `if(*l)` is false only on the very first call, before square 0 has been
set to the border value -1, so that call runs the game loop and every later
call is a search. Its four parameters change meaning with the job: for the
game loop the first one is `argc`, for a search they are the parent's score,
the depth, the parent's best score and the maximum depth. `simple.c` splits
this into `main()`, `search()`, `try_move()` and small helpers.

2. Macros that hide syntax

`_` is `if(`, `Z` is `while(` and `F` is `getchar())`, so the parentheses in
the source never seem to match. `v` is `pain(0,0,0` with the last argument and
the closing parenthesis left out, so `v,1)` is `pain(0,0,0,1)` and `v+1,1)` is
`pain(0,0,0+1,1)`. `H(z)` adds one character to the line buffer.

3. A large macro used as a function with an "undo" argument

`I(H,n)` is about 20 lines long and is expanded 11 times. It tries the move to
square `H`: it checks the target, scores the move, makes it, calls the search
again, checks for a cutoff and takes the move back. The second argument `n` is
code that is run every time the move is taken back, for example
`l[u-1]=-2^e` puts back a pawn taken en passant. Most uses leave it empty, as
in `I(p,)`. The macro contains `return` statements, so it can leave `pain()`
from the middle of a loop. In `simple.c` it is the function `try_move()`, and
the caller does the undo step.

4. One string holds every table and message

The string `O` is XORed with 3 when the program starts (`O[h++]^=3`), so none
of its text can be read in the source. After decoding it holds the piece
letters (`KQRBNP .pnbrqk`), the starting back rank, the piece values, the
centre bonus table, the knight steps, the king/queen steps, the column header,
`You win`, `I win`, `Stalemate`, `Illegal` and the `printf` format. The program
finds each part by its position, for example `O+106` is `Illegal` and
`O[28+r]*9-288` is the value of piece `r`. Numbers are kept as letters with an
offset, so the step `21` is stored as `80-21`, the letter `;`.

5. Pieces stored with a bit trick

Black pieces are 1 to 6 and white pieces are the bitwise complement `~1` to
`~6` (-2 to -7). The variable `e` is either 0 or -1 (all bits set), so
`l[u]^e` gives the piece type of the side to move as a number from 1 to 6
and anything else for empty squares, the border and enemy pieces. `pain()`
flips `e` with `e=~e` when it starts and when it returns, so the same code
plays both colours.

6. A board with a border

The board `l[]` has 10 columns and 12 rows. Only squares 21 to 98 with a
column from 1 to 8 are real squares; all the others hold -1. A knight or a
sliding piece that walks off the board lands on -1 and stops, so the program
never has to check the row or column of a square.

7. Cascading decrements instead of a switch

`q=l[--u]^e; _!--q){...} _!--q){...} _ 0<q&4>q){...} _ 4==q){...}` picks the
code for a pawn, a knight, a bishop/rook/queen and a king by lowering `q` each
time. The value left in `q` (piece type minus 2) is then reused inside the
score as a small penalty for moving more valuable pieces.

8. Ternary, comma and precedence puzzles

The score of a move is one expression with three nested `?:` operators, an
assignment to `d` and an `|=` to `S` hidden in the middle. Conditions use `&`
and `|` instead of `&&` and `||`, and rely on `&` being stronger than `|` but
weaker than `==`, as in `e&!(S&24)|!e&!(S&3)&&k`. `!(!((x-u)%10)|r)*99` is
99 only for a diagonal pawn move to an empty square, which is an en passant
capture.

9. Swapped operands

Comparisons and array indexes are often written backwards: `21==u`,
`1e4>v`, `u[l]` instead of `l[u]`, `10[s]` instead of `s[10]` and
`70[s]=~(*s=*n++-+84)`, which fills the first and last row of the board at
the same time. `a[b]` and `b[a]` mean the same thing in C.

10. Many values packed into one integer

`S` holds six castling flags in its low bits (has a king moved, has the a1,
h1, a8 or h8 rook square been used) and, from bit 6 up, the square of a pawn
that just moved two steps and can be taken en passant
(`(20==d)*64*x` and `S>>6`).

11. `setjmp`/`longjmp` to play a move

To play a move the program searches all moves with `pain(0,0,1,1)`. When it
reaches the move that was typed in (or chosen by the computer) and the king
is safe afterwards, it calls `longjmp(z,1)` straight from the middle of the
search. The move is then never taken back and `e` is never flipped back, so
the board, the castling flags and the side to move are all updated by
escaping from the function. If the search finishes normally, the move was
illegal.

12. Code that depends on the compiler

The castling test `o(u)&o(u-1)` should check that the king is not attacked on
its own square and on the square it passes over. Both `o()` macros store their
answer in the same variable `q` and read it back with no fixed order between
them, so with gcc only the second answer is used. `simple.c` keeps this
behaviour in `castling_path_safe()`. Both checks still run because each one
calls the search, which can call `rand()` and change later computer moves.

13. Hidden recursion depth and random choices

The number of command-line arguments sets the search depth through
`pain(0,0,0,2+L)` where `L` is `argc`. Equal moves at the top of the search are
chosen with `1&rand()`, but `srand()` is never called, so the computer plays the
same game every time it gets the same moves.

14. Missing headers and odd types

Only `<setjmp.h>` is included; `puts`, `printf`, `getchar` and `rand` are used
without being declared. `G` and `C` are typedefs for `int` and `char **`, and
`main()` passes `argv` and `envp` into `pain()` where they become unused
`int` parameters.
