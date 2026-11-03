# 1992/vern

Difficulty: Hard

1. Source rewritten by `sed` at build time

The real program is not `vern.c`. The Makefile runs `sed` to turn `{ ` into
`(`, `} ` into `)`, `}<tab>` into `=`, `{<tab>` into `i`, `|<tab>` into `ii`
and `;<tab>` at the start of a line into `#define `. So `V{ {	}	0; {	<M; ++{	}`
is really `for(i=0;i<M;++i)`. The braces you see are almost never blocks, and
you have to work out which whitespace follows each one before you can read a
line.

2. Keyword macros

`A` is `return`, `B` is `if`, `b` is `else`, `V` is `for`, `u` is `while`,
`U` is `int`, `R` is `double` and `H` is `printf(`. Every keyword is one
letter, so a line like `B{ !S} A J;` (`if(!S) return J;`) looks like noise.

3. Expression macros

`m(x)` is the sign of `x`, `a(x)` is the absolute value, and `h(x)` is how far
a row or column is from the nearest edge (0,1,2,3,3,2,1,0). `g` expands to
`tj()-J`, which is non-zero when another piece blocks a sliding move. `f` is
9999, used both as "infinity" for scores, the size of the arrays and the value
of the king.

4. Meaningless one and two letter names

Every function and variable has a name like `k`, `z`, `e`, `l`, `c`, `bb`
(pawn, rook, king, queen, bishop, knight), `I` (is a move illegal), `s` (is
the king in check), `C` (score the board), `PX` (search), `O` (move a piece),
`Y` (side to move), `F` (search depth) or `_` (piece type). Nothing in the
name tells you what it is for.

5. Board made of function pointers

The board `q[]` does not hold piece codes. Each square holds a pointer to the
function that checks moves for the piece standing there, and a second array
`P[]` holds the colour. To find out what piece is on a square the program
either compares the pointer with a function (`Z==z` means a rook) or calls the
function and reads the side effect it leaves in `_`, which is then turned into
a letter with `_+93+Y*16`.

6. Globals used as function arguments

The move checkers take no arguments. `I()` stores the source and target in the
globals `W` and `J` and the distance moved in `v` and `w` before calling
`q[W]()`. `TT()` sets `v=w=0` and calls the checker only to get the piece
letter. The search passes its alpha-beta bound to the next level through the
global `o`, and the chosen move comes back in the global `p`.

7. Moves made by brute force

There is no move generator. The search loops over all 4096 pairs of squares
(`i>>6` and `i&63`) and asks `I()` whether each one is legal. The scoring
code does the same to count how many squares each piece can reach. The check
test `s()` finds the king and then asks whether any enemy piece can "move"
onto it, restarting its loop with `i=-1` once the king is found.

8. Try a move with a callback

`QL(W,J,D)` makes a move, calls the function `D` passed to it, takes the move
back and returns the result. Passing `s` checks if a move leaves the king in
check, and passing `PX` makes the search recursive. The castle rook is moved
back by the same function `O()` that moved it, because `O(J,W)` sees a king
going two squares the other way.

9. Board constants computed with random numbers

`64`, `7` and `3` never appear. `UC()` adds random numbers until the total
passes 1 and returns the last one. This is the "inspection paradox": the
number that crosses the line tends to be bigger than average. Adding 10000 of
them and dividing by 100 gives about 64, which is rounded with `M&3` and `M&1`,
then `N` (7, the column mask) and `K` (3, the row shift) are worked out from
it. These are fixed constants in simple.c.

10. Bit tricks for rows and columns

A square number is `row*8+column`, so the program writes `J>>K` for the row,
`J&N` for the column, `i>>K+K` for the source square of a move number and
`i&M-1` for its target. Squares are typed in octal so the two digits are the
row and the column, and the board labels are printed with `%4o`.

11. Combined boolean tests

The move rules are single long `||` and `&&` chains, for example
`v*v*v-v||w*w*w-w` means "the king moved more than one square", and the pawn
rule `(J>>K)-K+(Y-1)/2` checks that a two square step lands on the right row
for either colour.

12. Order of argument evaluation

The prompt is printed with `H"...",X,T,C{ },s{ }...}`. `C()` changes `X` while
the arguments are being worked out, so the number shown depends on the
compiler evaluating the arguments from right to left.
