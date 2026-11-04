# 2006/night

Difficulty: Terrifying

1. Four-letter alphabet for every name

Every macro, variable and function is spelled with only the letters `G`, `A`,
`T` and `C`, like DNA bases: `CAA`, `CTG`, `TGA`, `CGAC`, `GTCA`, `AAG` and so
on. With so few letters the names all look alike, so it is very hard to
remember which one is the solver (`TGA`), the genetic algorithm (`ACC`), the
drawing routine (`CGAC`) or the winning animation (`GTCA`).

2. Macros that hide keywords and library calls

`G(G)` is `while`, `GA` is `return`, `GAT` is `int main()`, `CC` is `getch()`,
`CCC` is `rand()`, `TCA` is `clock()`, `AAA` is `refresh()`, `TAG` is
`endwin()` and `ATG` is the whole curses start-up sequence. `T` is first
`typedef unsigned` (giving the types `G` for `unsigned long` and `CT` for
`unsigned short`) and is then redefined as a function-like macro, so the same
letter means two different things in different parts of the file.

3. Overloaded symbols

`G` is a type, the `while` macro `G()`, and the parameter name inside that
macro. `A` is a global variable, a function parameter and the code-doubling
macro `A()`. `T` is a type macro, a stringizing macro, a global and a local
variable. The reader has to check whether a name is followed by `(` to know
which one is meant.

4. Unrolling loops with a doubling macro

`#define A(A) A { A }` writes its argument twice. Nesting it, as in
`A(A(A(A(...))))`, repeats code 16 times, which replaces the loop over the 16
maze cells, and six levels repeat one `mvaddch` line 64 times for each frame
of the rotation animation. In `TGA` and the move code it simply calls `GC()`
twice, once for the vertical and once for the horizontal direction, because
`GC()` divides the global `CA` by 2 on each call. The preprocessed program is
over 300 KB long. The rewrite uses ordinary `for` loops and an `axis`
variable.

5. `while` used as `if`

Statements like `G(!A)GA ...;` (`while (!A) return ...;`) and
`G(TC>>6)GA 1;` can only run once because the body leaves the function, so
they are really `if` statements. In `ACC` the loop
`while ((TC = TGA(GG = AG)) > CAA) { CAA = TC; ... }` also runs at most once,
because the second test computes the same value again.

6. `^` instead of `!=` and short-circuit operators instead of `if`

`G(++C^T)` means "while `++C != T`". Many statements are chains such as
`GC() && (AA[T] = ..., GT[T++] = ...)` or `TG&1 && (A ^= ...)`, where `&&`
and `||` decide whether the right side runs, just like an `if`.

7. The source code stringized into a data array

`#define T(T) #T#T#T#T#T#T T` turns its argument into a string six times and
then also leaves the argument as real code. The argument is most of the
program, starting in the middle of the declaration `GTT[]=T(` and ending at a
`)` deep inside `main`, so the same text is both the program and the string
`GTT`. The macro call cuts right through the brace structure of `main`.

8. A hidden message encoded in the letters of the code

At start-up `main` scans the stringized code in `GTT`. For each letter that
has bit 6 set and bit 3 clear (here `G`, `A`, `T`, `C`) it takes bit 2 of the
letter (1 for `G` and `T`, 0 for `A` and `C`) and packs eight of these bits
into each byte, in the odd bit order 0, 5, 2, 7, 4, 1, 6, 3 produced by
`A += 5` and `A % 8`. The bytes are written back over the start of `GTT`
itself. Every second byte from offset 302 spells `CONGRATULATIONS` followed
by a tab, which is shown only when the 64-move maze is solved. The rewrite
stores the decoded string directly.

9. Macro arguments without parentheses

`GG(T,C)` expands `C%2+1` without parentheses, so the call
`GG(TT^TG, GTC=TT)` becomes `COLOR_PAIR(GTC=TT%2+1)`. This silently sets
`GTC` to 1 or 2 instead of the cell number, which makes the later test
`GTC && A&2` the same as just `A&2`. It looks like cell 0 is special but it
is not.

10. Packed maze representation

The whole 4x4 maze is one 32-bit number: 2 bits per cell, cells numbered
along a snake path (left-to-right on one row, right-to-left on the next).
The position and the move counter share one number too: the low 4 bits are
the cell and the upper bits count moves (`GT[C]+16`). Moving is done by
XORing the cell number with 1, 3, 7 or 15, and rotating a piece by XORing
one bit chosen with `AT()`, which makes the values go 0, 1, 3, 2 like a Gray
code. `CG` tracks how the snake numbering maps to screen up/down/left/right
by flipping bits as it walks from cell to cell. The rewrite names these
pieces with helper functions such as `piece_at`, `rotate_piece`, `can_move`
and `neighbour_step`.

11. Bit tricks hiding simple tests

`AT(A)` is `A%4 >> A%4-1`, which is 1 only when `A%4` is 1 or 2 (and shifts
by -1 when it is 0). `72&T^64` is zero only for letters in the ranges
`@`-`G` and `P`-`W`. `CAA/4&16` is 16 when the best maze needs 64 moves.
`AGG` picks the expected key `w`, `s`, `d` or `a` with nested `?:`. The
rewrite uses `is_middle()`, named constants and plain comparisons.

12. Recursion instead of nested loops

`CGAC()` draws a two-dimensional sprite by calling itself: one level walks the
rows and, for each row, calls itself again with the arguments swapped to walk
the columns. `GTCA()` reveals the message one letter at a time by calling
itself with `C+3` and `A-1`. The rewrite draws sprites with two `for` loops.

13. Sprites and animation frames packed in one string

The string `ACG` holds 44 bytes of bitmaps for the rotation animation (each
byte is one column, each bit one row), the characters of the arrow pieces
with NUL bytes meaning "transparent", and at offsets 128-143 a table of
offsets into the same string for each sprite row. Rows are shared between
sprites to save space. The rewrite stores the four arrows and the animation
frames as small arrays of strings that look like the pictures.

14. Library calls with side effects hidden in arguments

`AC(G)` is `init_pair(G/2, G, nodelay(stdscr, G/3))`: it switches
non-blocking input on or off and uses its return value (`OK`, which is 0) as
the black background colour. `GTCA(GCA, ...)` passes the result of
`clear()` (also 0) as the starting letter number.

15. Delays and getch in odd places

Waiting is done by busy loops comparing a deadline to `clock()`
(`G(CG>TCA)A;`), assuming `CLOCKS_PER_SEC` is 1000000. The variable `C`
stores the key minus 114 (`r`), so `!C` means "restart", `C+1 == 0` means
"quit" (`q`), and after a move `C` is reused as the animation timer so that
the second direction test cannot match. The animation even declares a new
local `G C;` inside a block so the inner `C` and the outer timer `C` are
different variables.
