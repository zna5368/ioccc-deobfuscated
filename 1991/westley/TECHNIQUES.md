# 1991/westley

Difficulty: hard

1. Code shaped like the data it works on

The tic-tac-toe board is drawn in the source code. Each row of the board is a
function (`a`, `b` and `c`) and each cell is the space between two tabs in a
line like `X=-	X	-1;F;`. When the function runs, the cell becomes part of an
expression, so the board is read by executing it. In `simple.c` the board is an
ordinary `board` array of characters that `read_board` walks through.

2. Comments that hide half of a line

Each row is written twice, side by side, with `/*/` in it. `/*/` starts a
comment, but its own `*/` is not seen as the end because the `*` was already
used to open it. The comment only ends at the next `*/`, which is inside the
second copy. So the compiler sees one function while a reader sees two copies
on every line. On the last line `}` closes the function and the rest,
`/*/		-1;F;}/*/`, is one more comment. `simple.c` has no such tricks.

3. Characters used as numbers in expressions

`X=-	<cell>	-1` with the parameter `X` equal to 1 gives `- -1` = 1 for an empty
cell, `-X-1` = -2 for an `X`, and `-0-1` = -1 for a `0` (zero, not the letter
O, so it is a number in C). `S(&X)` then turns -2 into an `X` bit and -1 into a
`0` bit. Other characters are written as their ASCII codes: `P(34)` is `"`,
`P(44)` is `,`, `n+97` is `'a'+n`, `53` is `'5'`, `88`/`48`/`32` are `X`, `0`
and space, and `57-*v[u]` is `'9'` minus the digit typed. `simple.c` uses
character literals.

4. Macros in the compile line

`P`, `N`, `Q`, `F`, `J`, `K` and `H` are given with `-D` in the Makefile, so the
source cannot be read on its own. `F` is `S(&X)`, `J` is `z[4]` (the memory
line), `K` is the face character and `H` even declares the whole `w` table
inside the variable list `int q,i,j,k,X,O=0,H;`. `simple.c` defines everything
in the file.

5. Self-reproducing program (quine)

The program prints its own source with the new moves added. All of the code
after the board is stored again as strings in `z`, and `main` prints the board
rows, the array (once without quotes for `char z[][100]={` and then each string
in quotes) and the code lines in the right order. To read the program you have
to notice that the strings and the real code are the same text. `simple.c`
keeps the strings (they are the output) but names them `source` and prints them
with `print_board_row` and `print_source_line`.

6. Bit masks for the board

The positions of the `X`s and `0`s are stored as 9-bit numbers, cell 1 being
bit 8 and cell 9 being bit 0. The winning lines and move templates are also
bit masks in `w`, written in decimal (`146`, `7`, `292` ...) so their shape is
hidden. `~A&w[--j]||(q=0)` means "if every bit of line `j` is in `A`, a line is
complete". `simple.c` writes the masks in octal so each digit is one row of the
board, and names the check `has_winning_line`.

7. One global flag doing many jobs

`q` is "no win found yet" inside `y` and `e`, but it is also "print with
quotes" inside `s`, and is set by `q=i<12` in the printing loop. `i` is both the
cell found by `e` and the line counter in `L` and `M`. `e` relies on the
strange reset `i-=i*q` (set `i` to 0 only when `q` is 1) and leaves the answer
in `i` one past the found cell, which is why the move is `1<<--i`. `simple.c`
uses separate variables and functions that return their result.

8. Strategy hidden in a string that changes itself

The string `"9999999999  :-| "` is the computer's memory. Character 0 is the
opening move, characters 1-9 say which template to start with for each
opening (`J[*J-48]-40` turns `'9'` into 17) and character 14 is the mouth of
the face. When the human wins, `J[*J-48+(X=O=0)]--` clears the board (hidden
inside the array index) and lowers that digit, so the next program plays
differently. `simple.c` names these parts `FIRST_MOVE`, `FACE_MOUTH` and
`memory`.

9. Conditional and comma operators instead of if statements

`main` decides everything in one expression:
`q?e(O,X),q&&(K='|'),e(X,O),R(),O|=1<<--i:J[...]--;`. The `?:` picks between
"human won" and "computer moves", the commas run several steps, and `q&&` /
`||` stand in for `if`. Every later step does nothing once `q` is 0, which is
how "stop at the first move found" works. `simple.c` uses `if` statements and
early `return`s in `choose_computer_move`.

10. Old style function definitions and implicit int

Functions like `S(x)int*x;{...}` and `main(u,v)char**v;{...}` use the pre-ANSI
style with no return types, and `putchar` is never declared. `simple.c` uses
prototypes and declares every type.
