# 2000/robison

Difficulty: Hard

1. Macros that hide keywords and repeated code

`#define F return`, `#define C if(`, `#define G typedef` and `#define A B.`
replace common keywords with single letters. `J(E,K)` is a counting-down loop
`for(E=K; E--; )`, `D` is `>>E&1` ("is bit E set") and `H` is `printf( I(`,
which also hides that every `printf` format goes through the decoder in
technique 6. `L(M)` hides the material count used by the evaluation:
`N(O.M)+3*N(O.M&O.P)`.

2. One letter types, names and reused letters

`Q` is `int`, `R` is `unsigned` and `W` is the board structure, whose fields
`N, S, P, T, U, V` are mine, theirs, kings, result, alpha and beta. The same
letters are also the names of functions (`N()`, `S()`, `P()`, `T()`, `U()`,
`V()`), so `A N=N(a.S)` reads as nonsense. Inside `I()` a local `int` is named
`W` and another is named `B`, the same as the board type and the board
variables everywhere else.

3. One line per token ("linear programming")

The source is split into hundreds of lines holding only a few characters
each, even in the middle of expressions, numbers like `4095` and
`char` constants (using backslash-newline line splicing, as in `'\` `\0'`).
Reading it top to bottom gives no sense of where a statement starts or ends.

4. `~-` and `-~` instead of `-1` and `+1`

In two's complement `-~x` is `x+1` and `~-x` is `x-1`. So `-~E` prints the
square number one higher, `~-Y` is the reduced search depth and `~-(b&2)`
is `(b&2)-1`. `~E` (zero only when `E` is `-1`) is used as "is not -1", and
`!~g` means `g == -1`.

5. One function that reverses or counts bits

`N()` combines neighboring bit groups of 1, 2, 4, 8 and 16 bits with masks
`-1/(Y+1)` (0x55555555, 0x33333333, ...). The multiplier `Y*_-~-_`
depends on the global `_`: when `_` is 1 the multiplier is `Y`, which swaps
the groups so the 32 bits are reversed (used to rotate the board for the
other player); when `_` is 0 the multiplier is 1, so the groups are added and
the result is the number of set bits (used to count pieces). Before each use
the code quietly sets `_` (`_=1` in `S()`, `_=0` in the evaluation), and the
same global `_` is also used for "the piece must jump again". simple.c uses
two separate functions.

6. Encrypted strings made of whitespace

Every message is stored as a string made only of space, tab, `{`, `}` and
`;` characters, which the contest size rules do not count. `I()` turns each
character into a few bits (`E%65%6^2`), packs them together 7 bits at a time
and writes the real text into the buffer `l`. Several strings contain an
embedded `\0`, so adding an offset selects the second string, for example
`"Red\0Black"+o/2*2` and `"\0jump "+U(...)`. `" \0%c"+2` chooses between
printing a blank (light square) and printing the piece (dark square).

7. Board stored as bit masks with arithmetic geometry

Each side is a 32 bit `int`, one bit per dark square. The square in a
direction is computed by `P()` as `Y+(b+8)%10-5+Y/4%2&31`, which gives steps
of +3, +4, -5, -4 plus one on odd rows, then checks that the result is on the
next (or previous) row. The board is always kept from the view of the player
to move; `S()` bit-reverses the board (rotating it 180 degrees) and swaps the
sides, which is why the computer prints `32-(Z&31)` and `Z>>5&3^3` to turn
its move back into the human's numbering.

8. Moves packed into one number

A move is `128|b*32|a`: the starting square in the low 5 bits, the
direction in the next 2 and a jump flag in bit 7. `j>>7` tests for a jump
and `Z>>7?V(...)` makes `V()` call itself to do the second half of a jump,
which also removes the captured piece by clearing the square it moved over.

9. Nested `?:` and comma expressions instead of `if` statements

The search in `f()` is one huge expression:
`I=_?f(...).U:(Y=b+U(O=S(O),-1,1))?-(d(O,~-Y).U):(_=0,Z L(N)k L(S)...)`.
It chooses between continuing a multiple jump, searching one level deeper
(with an extra level when the opponent must capture) and evaluating the
position, and it assigns several variables in the middle of the expression.
`T()` uses three nested `?:` to return a jump, a simple move or `-1`.

10. Combined alpha-beta bounds

The `U` and `V` fields of the board hold the best score so far and the
cutoff value. `S()` passes the negated best score of the parent as the
child's cutoff (`A V=-a.U`), so one `B.U <= B.V` test does both the alpha
and beta cutoffs of the negamax search, and the result is negated with
`-(d(...).U)`.

11. Unused variables and dead stores

`f()` counts the legal moves in `i` and stores the count in `_`, but the
value is always overwritten before it is read. The global `_` also gets set
in `S()` and in the evaluation only for the benefit of `N()`.

12. State packed into bits of one variable

`o` holds whose turn it is (bit value 4 means the human) and whether a
multiple jump is being continued (bit value 1). `o/2*2` clears the jump bit,
`^4` switches the player, `o<4` means "computer" and `o/2*2` is also the
offset that selects `"Red"` or `"Black"`. The variable `g` is reused both as
the "input was illegal, ask again" loop counter and as the saved previous
state.
