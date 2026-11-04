# 2011/hamaji

Difficulty: Terrifying

1. Source code drawn as a picture

The whole program is laid out as a solved nonogram that shows a dragon. The
numbers at the top and left of the file are the puzzle's clues, and they sit
inside comments (`/* ... */` and `//`). The code itself forms the black cells.
Because of this, statements are split at odd places, and comment markers such
as `/*` at the end of one line and `*/` at the start of the next hide the clue
numbers between pieces of code.

2. Padding to fill the picture

Extra empty statements `;;;;`, stray expressions like `z*9;`, `0,` inside a
comma list, `000,` and `(00, C[i]+k)` do nothing. They are only there to take
up the right number of characters so the code matches the dragon's shape.

3. Short global variable names

Every variable is one letter and global (`C`, `R`, `W`, `H`, `J`, `K`, `B`,
`S`, `T`, `U`, `O`, `Z`, ...). The same variable is used for unrelated jobs.
For example `Z` holds the space states of each line while solving, and later
the length of each clue string while printing. `K` holds the result of
`scanf`, then is the cell counter for guessing, then the width of the row
clues. `q` points first at `H`, then `W`, then `K`, then `J`.

4. Negative logic bit sets

Each cell keeps a 64-bit number where each bit is one possible "state" of the
line (inside which block or gap we are). A bit set to 1 means the state is
**not** possible. This lets the zero-filled global array mean "anything is
possible" without any setup code, but it makes every test look backwards,
like `~S[z] & O[i]` or `!~v` (true when every bit is 1, so no states are left).

5. Lines as state machines built with bit shifts

`O[i] |= (1LL<<v)-1<<k` and `Z[i] = z |= 1LL<<k++` build masks of "box"
states and "space" states for each line from its clues. The two passes in
`for(j=-61;++j<60; v=(v|~z)&(j<0?v>>1:v<<1|1))` walk the line backwards
(for negative `j`, using `~j` as the position) and then forwards in a single
loop, moving the state set one step with a shift each cell.

6. Rows and columns stored in the same arrays

Rows use lines 0 to 59 and columns use lines 60 to 119 of `R`, `S`, `O`, `Z`
and `C`. Expressions like `B[i<60?i:j][i<60?j:i-60]` and
`k=i<60?j+60:j` swap between "row view" and "column view" of the same cell.

7. Pointer flipping between buffers

`S -= t *= -1` negates `t` and moves `S` back and forth between two halves of
`T`, so the program can compare the new pass with the old one in `D()`.
`S += J = 14400` jumps to two more spare buffers for trying out guesses.

8. Complex nested conditional expressions

Most logic is written as chains of `?:` operators with assignments hidden
inside. For example
`B[..] = ~S[z]&O[i] ? ~S[z]&Z[i] ? r=0 : (U=O,1) : (U=Z,2)` decides the cell,
sets the status `r`, picks a mask in `U` and stores the answer, all in one
expression whose value then decides whether the crossing line is updated.

9. Input parsing packed into a for loop header

`for(;K=scanf(...)<1?q=&W,j--,2>++c:'\n';j=-K+getchar()?j+1:++*q*0);` reads
all the clues with an empty body. The value of `K` is the character expected
next (`'\n'` after a number, `1` after the `-` line), so `-K+getchar()` is zero
only at the end of a line, where `++*q*0` both counts the line and resets `j`.

10. Output loop with negative indices

The printing loop starts `i` and `j` at `~J` and `~K` (negative numbers). Negative
rows print the column clues, negative columns print the row clues, and
`"?X "[B[i][j]]` indexes a string literal to print unknown, box, or space.

11. Hidden status codes

The variable `r` holds 0 (unsolved), 1 (changed), 2 (contradiction) or 3
(solved). The end check `r&1||puts(r?"invalid":"failed")` uses the low bit
and a nested choice instead of naming these cases.
