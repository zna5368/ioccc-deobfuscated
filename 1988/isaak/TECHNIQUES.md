# 1988/isaak

Difficulty: Terrifying

1. The file includes itself

`#include __FILE__` makes the preprocessor read the same file a second time
inside the first pass. `#if U defined(y)` chooses which half of the file is
used on each pass. `y` is defined on the command line (`-Dy`), so the outer
pass takes the `#if` branch, removes `y` and includes the file again. The inner
pass now takes the `#else` branch, which holds the data strings and globals.
`U` starts out empty and is redefined to `!` near the end of the inner pass, so
the final `#if U defined(N)` is false on the inner pass and true on the outer
pass. That is how the `exit(N)` function only gets compiled once.

2. One piece of code compiled twice with different macros

The code after the `#endif` is read once on each pass. On the inner pass the
command line gives `-DI=B -DO=-`, and on the outer pass the file switches to
`#define I K` and `#define O +`. So the same text produces two pairs of
functions:

- `Y` (inner) and `Pb` (outer) draw half of a box each. `F P(O,=)-263` becomes
  `F -= -263` in one and `F += -263` in the other, so one draws the bottom and
  left edges moving backwards and the other draws the top and right edges
  moving forwards.
- `P(I,r)` becomes `Br` (inner) and `Kr` (outer). `*IP(O,O)` becomes `*B--`
  or `*K++`, `21 O -5` becomes `26` or `16`, and `81 O 16` becomes `65` (`'A'`)
  or `97` (`'a'`). `Br` makes the upper case first letter of a symbol and `Kr`
  makes the lower case second letter.

3. Token pasting to build operators and names

`#define Pa(x,y) x##y` with `#define P(x,y) Pa(x,y)` glues tokens together
after the macros inside them are expanded. `P(O,=)` gives `-=` or `+=`,
`P(O,O)` gives `--` or `++`, and `P(I,r)` gives the function names `Br` and
`Kr`. The function names `Y` and `Pb` are written alone on a line far away
from the `()` and body that follow them.

4. Replacing `exit()` and renaming `main`

The first line `main(){exit(0);}` looks like a program that does nothing.
The real program is a user written function called `exit(N)`, which replaces
the library `exit()`. On the second pass `#define main Si` renames the
included copy of `main` to `Si` so there are not two `main` functions. The real
`exit` stops itself with `_exit(0)` hidden inside the loop condition
`*H||(_exit(0),0)`.

5. Encoded symbols with a hidden escape character

The two strings `K` and `B` hold the chemical symbols. The first letter is
`'A' + (byte of B % 26)` and the second letter is `'a' + (byte of K % 16)`.
Since 16 letters are not enough, `'k'` acts as an escape: when it appears the
low 4 bits of the next byte of `K` are added to it to reach `'k'` to `'z'`.
A second letter of `'j'` means the symbol has only one letter and is printed as
a space (`*H==106?32:*H`).

6. One string used for two jobs

`K` is read forwards for the second letters of the symbols (low 4 bits of each
byte) and then read backwards for the atomic weights (high 4 bits of each
byte). The weights are stored as differences from the previous weight
(`V+=...`). A difference of 1 to 5 is stored directly, and a high nibble that
gives 0 is an escape that looks up the real difference (-2, -1, 0, 6, 7 or 12)
in `"hijpqv"` using the byte before it. The backward walk stops at the tab
character inside the string.

7. A macro that hides operator precedence

`#define C K/16-2` has no parentheses, so `*C` becomes `*K/16-2` which is
`(*K)/16 - 2`, the high nibble of the current byte minus two. `*--C` becomes
`*--K/16-2`, which also moves `K` back one byte.

8. Two stage layout drawn with pointer moves

The upper bytes of `B` (`*B++/26-1`) are movement codes. Each symbol is first
written along a snake shaped path in a wide 32 column table with the
lanthanides and actinides inside periods 6 and 7: `1` moves one cell left, `2`
one cell right, `3` one period down and `0` jumps back to the top left. The
offsets come from the string `")21["`. The second stage scans the buffer from
start to end, so the symbols are found in order of atomic number, and moves
each one to its final place. The shift is picked by the column with
`"(\206/"[((H-S)%130+45)/57]`: no move for the first three columns, `+1296`
(10 lines down and one cell left) for the lanthanides and actinides, and `-56`
(14 cells left) to close the gap for the rest of the table.

9. `sprintf()` tricks

`sprintf(H=S,"%5060d")` is given no number to print. It is only used to fill
the 5202 byte buffer `S` with spaces using a field width of 5060. Each output
line is copied by `sprintf(F+3,"%c%-73.73s\n",0,H)`. The `%c` with `0` puts a
NUL byte first so that `write(1,F+4,...-1)` can print the rest using the
return value of `sprintf()` as the length. This scratch copy is written inside
the picture itself, next to the last element drawn, and its leading NUL is what
cuts that line short when it is later printed.

10. Magic numbers for the grid

The buffer is a grid of 130 character lines, so `+130`, `-131`, `+129`,
`-263` and `-132` are moves of one line plus or minus a few columns.
`(*H+5&129)+1` turns `'+'` and `'-'` into a move of 1 and `'|'` into a move of
130, so the string `"$+---+|||"` draws an edge and then a side of a box.
`!(*++H+5&64)` skips every character that is not a letter.

11. Declarations hidden in a long list

`char*K="...",*H,S[5202],*B="...",*F,N;` mixes the data strings, the buffer
and the work variables into one declaration that is split across lines in the
middle of the string literals.
