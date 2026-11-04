# 2011/hou

Difficulty: Terrifying

1. Keyword and library names redefined as macros

`return`, `puts` and `clear` are `#define`d to mean something unrelated. `puts(d,e)`
is not a print call at all: it ends the `if (argc<2)` block with `return 0;}` and opens
a new block that declares the local variables. `return` is redefined so that every
`return` first prints the result when `argc==2`. A reader who trusts the familiar
names is misled about what each line does.

2. ASCII art that is real code

The calculator drawing is not a comment. Each word in it is a macro or an operand:
`l` expands to `(r);r=--b?r:`, so `l -x l tan` becomes `r=--b?r:-(r); r=--b?r:tan(r);`.
`I (/)` expands into the check for a `/` in the input, `clear` hides the number
reading code, and the `+~~~~+`, `!` and `|` borders are parts of expressions that are
computed and thrown away (see technique 3).

3. Operators that cancel themselves out

The borders `+~~~~...~~~~+` hold an even number of `~` (bitwise NOT). Two NOTs undo each
other, so `~~x` is just `x`. Combined with `!1` (which is 0) or `*_*(int)-!7.584` (a
multiplication by 0) they add nothing to the value. Lines like
`c = c | ... | 2 | 3 || (r);` assign to a variable that is never read again.

4. Example input and output that are actually constants

The "usage" line `calculator 11/26+222/31` and the "display" `7.584,367` look like
documentation. After macro expansion `calculator` becomes `*_*(int)`, so the usage line
is the hash `ch*11/26+222/31` (that is `ch*11/26+7`). The comma splits the display into
the two macro arguments: `7.584` is multiplied by zero and `367` becomes `3*367 = 1101`,
the bitmask of which hash slots hold an operator.

5. Hash functions instead of a lookup table

There are no strings naming the operators. One hash `(ch*11/26+7)&15` puts each
character in one of 16 slots; slots 11 to 15 are digits (start of a number). A second
hash `(ch%21+7)%9` picks which function to run (1 negate, 2 tan, 3 reciprocal, 4 cos,
5 exp, 6 sqrt, 7 sin, 8 log), and the bitmask `1101` turns it off (0) for characters that
should do nothing. Only one letter of each function name matters, for example `i` in
`sin` and `q` in `sqrt`; the other letters are no-ops.

6. Button layout as a countdown switch

The functions appear on the "buttons" in the order `-x, tan, 1/x, cos, exp, sqrt, sin,
log`. Each `l` decrements `b` and applies the function only when `b` reaches 0, so the
position of a button on the panel is its operation number. This replaces a `switch`
statement.

7. Line numbers used as data

`usage` expands to `main(4-__LINE__/26,argv)`. The rows with `/` and `*` are on lines 23
and 25 (`__LINE__/26` is 0, so call level 4, which multiplies) and the rows with `+` and
`-` are on lines 27 and 29 (level 3, which adds). Moving those lines changes the program.

8. Recursive main with argc as the parser state

`main` is the whole parser. `argc` is reused to say which level is running: 2 is the
top-level expression that prints, 3 is an expression in parentheses, and 4 is a single
factor. The return value (`argc>=4`) tells the caller whether to multiply or add.

9. Unusual grammar

`-`, `/` and every letter are parsed as prefix unary operators instead of binary ones:
`a-b` becomes `a+(-b)` and `a/b` becomes `a*(1/b)`. This gives the right answer for
normal expressions while needing only `*` and `+` as binary operations.

10. Command line argument used as the cursor

`_` is `argv[1]`, and the program moves through the input by incrementing `argv[1]`
itself (`*_++`, `while(*++_-c);`) rather than using an index variable. The number reader
uses `sscanf` to read the number and the character after it, then skips forward until
it sees that character again.

11. goto loop

The binary operator loop is written as a label `g:` at the start of the button panel and
a `goto g` inside each `I(...)` macro, rather than a `while` loop.
