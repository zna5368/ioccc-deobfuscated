# 2019/duble

Difficulty: Insane

1. Single letter names that are reused

Every global is one letter (`a`, `b`, `c`, ... `n`, `A`, `B`, `w`, `x`), and the
same letters come back as function names and parameters: `r(int r)`, `t(int t)`,
`P(int P)`, `G(int G)`. Inside `main` the parameters are called `F` and `D`,
which hide the globals `F` (the message size) and `D` (a send address). So the
same letter means different things depending on where you are. The two message
structs `w` (received) and `x` (sent) have twelve fields named `A` to `L` with no
hint of what they hold (cursor column, row, color, mode, ...).

2. Hidden global side effects

The helper `r(fd)` looks like it only tests whether a descriptor is ready
(`FD_ISSET`), but it also stores `fd` in the global `b`. The send function
`G()` and receive function `H()` always use `b` as their socket, so the line
`if(r(4))` quietly picks the socket that the following `H()` reads from. You
have to track `b` through every expression to know which socket a message uses.

3. Message types as magic numbers

All communication between the editors uses one struct with a type number in
field `A`. The numbers 0 to 10 (join, welcome, file byte, file request, quit,
cursor update, commit, server handover, ...) never get names. `G(int G)` also
decides from the number alone where the message goes:
`D=G<5?0:G^8?&B:&A` means types below 5 use an already connected socket, type
8 goes back to the last sender and everything else goes to the server. Type 3
even sends the received struct `w` instead of `x`.

4. Inferred file descriptor numbers

The program never saves the numbers returned by `socket()` or `fopen()`. It
relies on Unix always handing out the lowest free number: the client socket is
always 3, the server socket is always 4 and the server's connection to its own
client is always 5. So `close(4)`, `read(4,v,u)` (reading a file opened with
`fopen`), `select(5,...)` and `x.B^5` only make sense once you know this.
`O()` even decides what address to bind to with `b==4?&B:&C`.

5. Keys reduced with a modulus

Each key byte is turned into `N%65`. That makes the last letter of the arrow
key escape codes `ESC [ A` to `ESC [ D` become 0 to 3, and the commands
become 34 (`c`), 43 (`l`), 44 (`m`), 47 (`p`) and 48 (`q`). The mode shown on
screen is printed as `x.J+65`, which turns 44/47/43 back into `m`/`p`/`l`.

6. XOR used as "not equal"

Comparisons like `34^N||(...)`, `a^1||(...)` and `N^47&&N^43||(...)` use `^`
(XOR) which is zero only when both values are equal. Combined with `||` and
`&&` these replace `if` statements: `a^4||(...)` means "if a is 4 then do
...".

7. Control flow packed into expressions

Almost the whole program is long chains of `,`, `&&`, `||` and `?:`. For
example the arrow key handling is
`x.F+=N==2&(x.F-=N==3&&x.F)!=W-3` which subtracts one for left (if not at the
edge) and then adds one for right (if not at the other edge) in one
statement. The `?:` with a comma inside the middle part
(`N==x.J?x.E=0,G(10),44:N`) commits a line and returns a new mode at the same
time.

8. Loops abused as program structure

`main` uses `for` loops with odd conditions instead of plain code. The loop
`for(...;F++-4;)` runs twice because `argc` is 2: the first pass turns raw
terminal mode on and runs the editor, the second pass turns it back off
(the termios flags are toggled with `^=`). The inner loop `for(...;F-4;)`
never ends by itself, only a hidden `break` leaves it. The line drawing is a
`for` whose condition part draws the point and whose body is the step.

9. Escape codes and data hidden in one string

The string `J` holds three pieces separated by `\0`: a black cell
(`"\e[0;30;40m "`), a row ending (`"\e[0m\n"`) and the file header
(`"#!/bin/cat\n\e[1A"`). `P(18)`, `P(12)` and `P(0)` pick them by offset.
The string `I` (reset, clear screen, home) is also reused as `I+8`, which is
only the "move cursor home" part. Cells in the drawing are changed by writing
digits at fixed byte offsets (`*(M-6)`, `*(M-3)`, `*M`, `*(M+2)`) into these
escape codes, so you need to count characters to see that they are the
attribute, foreground color, background color and character.

10. One algorithm for every mode

Moving, pen drawing and line drawing all run the same Bresenham line loop. In
move and pen mode the start and end point are equal so the "line" is a single
cell. Whether a cell gets painted depends on `47^w.J&&43^w.J|w.A^10`, a
hard to read test for "pen mode, or line mode when the line is committed".
The cursor itself is shown with inverse video so that a space displays the
foreground color.

11. Build time macros

`V`, `W`, `g` and `s` are not defined in the source at all. The Makefile
passes them with `-D`: the terminal size, the terminal flags to switch off
(`11` is `ISIG|ICANON|ECHO` on Linux) and the first byte of socket names
(`0` puts them in the hidden Linux abstract namespace). The `enum{u=...}` is
only there to make the buffer size a constant that can size a global array.

12. Misleading layout and comments

The code is shaped into a big picture, with words split across lines
(`.`/`sun_family`, `x.`/`F`). Comments such as `//IOCCC`, `//2019`,
`/*O()*/`, `// !b+12` and `//d+=i` look like they mean something but are
only decoration.
