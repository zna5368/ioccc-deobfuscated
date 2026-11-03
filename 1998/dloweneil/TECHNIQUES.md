# 1998/dloweneil

Difficulty: Medium

1. Source code shaped into a picture

The whole body of `main` is laid out so that the code spells `poot` in big
letters. Statements are split in odd places, e.g. `X = Y=  p=` on one line and
`s=  0;` on another, and a keyword like `for` is separated from its
parenthesis by blank space. The compiler ignores whitespace, so the shape has no
meaning. It just hides where each statement starts and ends.

2. Macros that hide whole statements and loops

`P(o,O,t)` is `move(o+3, O+3); addch(t);`. It draws a character at a board
position. `R` is a complete loop that redraws the board and the score, and `D(T)`
is a whole `switch` statement that draws a character beside the board. When you
see `D(c); R;` in the code, the loops and switches behind it are not visible.
`E` and `F` are shorthand for `x/Y` and `x%Y`.

3. A single counter used as a 2D coordinate

The loops run one variable `x` from `0` to `X*Y` and get the column with
`x/Y` (macro `E`) and the row with `x%Y` (macro `F`). This hides the fact
that the loops walk over a grid. The rewrite uses two nested loops for column and
row instead.

4. Several coordinate systems

There are three ways to describe a position. Curses screen positions are
board positions plus 3. Board cells are `L[column][row]`, while `P` takes
`(row, column)`, so the arguments seem to swap places between calls. The waiting
letter has a single number `p` that goes clockwise around the border, from
`0` to `2*(X+Y)-1`. The macro `I(n)` turns `p` into a side number (0 = top,
1 = right, 2 = bottom, 3 = left). It does this with
`2*(n/(X+Y)) - 1 + (n%(X+Y)-X < 0 ? 1 : 2)`, which is much harder to read than
a chain of `if` tests.

5. Formulas used instead of case analysis

When a letter is dropped, the starting cell and direction are computed with
expressions like `a = (n%2) ? X+(n/2)*(1-X)-1 : (1-n)*p+(n/2)*(2*X+Y-1)`.
Each one is really four simple cases, one per side of the board. The
multiplications by `n/2`, `1-n` or `n-1` just switch terms on or off for each
side. The rewrite puts this in `entry_point()`, with one plain branch per side.

6. A hash function on letter codes

`c%3` gives 2 for `P` (80), 1 for `O` (79) and 0 for `T` (84). So
`(c%3-1)*-1` gives -1, 0 or +1 depending on the letter. This value tilts the
direction of travel clockwise, straight or counterclockwise without ever naming
the letters. The rewrite uses a `switch` on the letter in `drift()`.

7. Numeric character codes

Characters are written as numbers: `32` is a space, `42` is `*`, `45` is
`-`, `124` is `|`, `43` is `+`, `84` is `T`, and the keys `97`, `115`
and `100` are `a`, `s` and `d`. You have to look them up in an ASCII table
to see what they mean.

8. Marking cells by adding a number

A letter that is part of a spelled word gets 9 added to it (`L[...] += 9`),
which moves it past `T` (84). The test `L[E][F] > 84` then means "this cell is
marked". That is why such cells are drawn as `*` (42), and why the word search
accepts `L[a][b]-9 == C[n]` as well. The same marking loop also counts the
score bonus by doubling `S` once for every marked letter.

9. `goto` used as a loop

`f:` labels the search for spelled words, and `goto f;` jumps back to it after
letters are removed. This repeats the search to catch chain reactions. It is
really a `while` loop, which is how the rewrite writes it.

10. Reused and misleading variable names

All variables have one or two letter names, and most are used for several
things. `t` and `u` are `argc` and `argv`. `x` is the grid counter, but it
also holds the key just pressed and the column index in the collapse loop. `n`
is a side number, then a loop counter, then a "found a marked cell" flag. `A`
and `B` are the direction of a falling letter and also the search directions.
The variable named `x` is not an x coordinate.

11. Hidden `else` binding and side-effect conditions

`if((!a)&&A<0)if(!B){ A=0; B=1; } else A*=-1;` relies on the `else`
belonging to the inner `if`. This "dangling else" is easy to misread. Lines like
`if(--p<0)` change a variable and test it in the same expression.

12. Word list that also sets the letter odds

`C="POOT"` is both the word to spell and the set of letters to pick from with
`C[random()%4]`. Because `O` appears twice, it comes up twice as often as `P`
or `T`, without any explicit probability code.
