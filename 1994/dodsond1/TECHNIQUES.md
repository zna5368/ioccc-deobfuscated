# 1994/dodsond1

Difficulty: Medium

1. Macros that rename keywords and operators

The source has almost no real C in it. Keywords are hidden behind one letter
macros such as `#define r for`, `#define E(x) if(x)`, `#define B(x) while(x)`,
`#define K do`, `#define H(x) return x` and `#define J int`. Operators are
turned into function-like calls: `O(x,y)` is `x+y`, `W(x,y)` is `x*y`,
`D(x,y)` is `x=y`, `V(x)` is `!x` and `u(x)` is `-x`. Because every
expression becomes a nested call like `D(h,O(h,m(f,g,1,1)))` (`h += m(f,g,1,1)`),
the code looks like Lisp instead of C, as the judges note.

2. Array indexing written as pointer arithmetic

`N(x,y,z)` expands to `*(*(x+y)+z)` and `M(x,y)` to `*(x+y)`, which are
just `x[y][z]` and `x[y]`. Even the string lookup that picks the board
character is written `M("X O<", ...)` instead of `"X O<"[...]`.

3. Constants hidden in macros

Numbers are spelled indirectly: `Q` is `u(1)`, which is `0 + -(1)`, so it means
`-1`. `n` is 60 (the number of turns), `o` is `'A'`, and `P` is
`random()%6`. The board and weight tables are built from macros `R`, `Z`,
`S`, `T` and `U` that expand to rows of numbers, so you cannot see the table
shapes without running the preprocessor.

4. Loops that count to a "not equal" condition

The macro `A(x)` is `for(x=1; x+(-9); x=x+1)`. The test `x-9` is non-zero
(true) until `x` reaches 9, so this is really `for (x = 1; x <= 8; x++)`.
Inside `m()` the direction loop also does `g = g - 1` at the start of the body
and `g = g + 1` at the end, so the loop counter 1..8 is used as array index
0..7 only while inside the body.

5. Equality tests written as sums

Instead of `a == b` the code writes `!(a + -b)`, and instead of `a == -b` it
writes `!(a + b)`. For example `V(O(N(a,...),j))` checks whether a board cell
holds the opponent's piece (`-j`). The test `E(O(k,Q))` (`if (k - 1)`) means
"if k is not 1".

6. One function with four jobs

The function `m(h,i,j,k)` checks whether a move is legal, is summed over the
whole board to see whether any move exists, places a piece, and scores a move.
The fourth argument `k` controls it: when `k` is 1 the captured pieces are
multiplied by 1 (nothing changes), when `k` is -1 they are multiplied by -1
(the pieces flip colour). The return value is the score of the move or 0 when
nothing would be captured.

7. Meaningless single letter names and reused variables

All variables are single letters (`a`, `b`, `c`, `d`, `e` ... `k`), and the
same letters mean different things in different functions. In `main()` the
variable `h` first holds the sum of all legal human moves and later the best
computer score, while in `m()` the same letter `h` is the row argument. The
turn counter `e` in `main()` is the move score in `m()`.

8. Sentinel border and number encoded pieces

The 8x8 board is stored in a 10x10 array with an outer ring of zeros, so the
loops that walk across the board stop at the edge without any bounds checks.
Pieces are stored as 1, -1 and 0, which makes flipping a piece a simple
multiplication by -1 and lets the printing code look the character up with
`"X O<"[cell + 1]`.

9. Source layout

The code is spread out with lots of spaces and empty `/***/` comments so it
forms the shape of an Othello board, and the old K&R style function header
`J m(v) J v;` (`int m(h,i,j,k) int h,i,j,k;`) hides the argument list behind
another macro.
