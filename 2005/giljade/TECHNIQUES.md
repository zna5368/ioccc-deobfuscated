# 2005/giljade

Difficulty: Insane

1. Source code shaped like the puzzle

The layout of `giljade.c`, with its indented lines and blank areas, is the
start position of the puzzle. The program reads its own source with `fopen(__FILE__,"r")` and prints
it again in the shape of every position, so the source file is also the
program's data.

2. Markers hidden in empty statements

The `;;` sequences do nothing in C. The program looks for them in its own
text. The first two are changed to `#` and mark where the macros end and where
`char*A=` starts. The second one also deletes the `0` nine characters later, so
`A=0` becomes `A=` and the printed copy can put a string there. The code between
the third and fourth `;;` (the code that reads the file) is erased, because the
printed copies keep their source in the string `A`.

3. A shell command hidden in a comment

`/*echo/Line/%d;sed/-n/-e/ %d,%dp/%s/|sed*/` looks like an odd comment. With a
command line argument, every `/` and `*` between the two markers is replaced
by a space. The result is a `printf` format string,
`echo Line %d;sed -n -e %d,%dp %s|sed -e ...>c.c;make c; rm -f c`, which is
passed to `system()` once for each printed program to compile it.

4. Macros with operators as arguments

`o(p,t,i,m,a,l)` takes `<<`, `>>`, `>>4` and even an empty argument. One macro
call creates all the moves in one direction, and the four calls
`o(1048560,<<,>>,4,>>4,1)` and so on cover up, left, right and down. `I(x,d)`
puts `x` after `(b)` so `N[d]^=+b^(b)x` turns into `N[d]^=+b^(b)<<(4)`.
`H()` computes the empty cells and `F` stands for `for`.

5. Bit boards

Each kind of piece (2x2, tall, wide, 1x1) is an `int` with one bit per cell,
where bit `row*4+col` is set for the top left cell of a piece. Shifting by 4
moves a row and shifting by 1 moves a column. Masks such as `489335`
(`0x77777`) and `978670` (`0xEEEEE`) stop pieces from moving off the side of
the board. ANDing the shifted empty cells with a piece mask finds every piece
that can move at the same time.

6. Lowest bit trick

`s=s^(b=s&s-1^s)` gets the lowest set bit of `s` into `b` and removes it from
`s`. `s&s-1` clears the lowest bit, and XOR with `s` leaves only that bit. It
is used to go through the movable pieces one at a time.

7. Hash table and queue in one array

`B[1<<21]` holds the hash table in the first million entries and the search
positions from index `1234567` on. Each position takes 6 ints: four piece
masks, the next position in the same hash bucket and the parent. `C()` walks a
bucket by jumping to field 4 of a position (`h=B[h]+4`) and only adds the new
position if it is not found. When the goal is found, the start of `B` is reused
as a stack for the solution path. Setting `N=B` there also ends the search loop
`E<N`.

8. Numbers instead of characters

`T=42` and `M=47` are `*` and `/`, `V=32` is a space, and numbers like `34`,
`35`, `59`, `92` and `100` stand for `"`, `#`, `;`, `\` and `d`. `40-(h&62)` is
zero only for `(` and `)`, and `h-125` checks for `}`.

9. One state variable for a big printer

The variable `O` holds nine different states (code, comment, string, filler
comment and so on) and `a` and `e` hold extra state. Everything is written with
nested `?:`, `&&` and `||`, for example `O?j?j-1&&(O-2&&d--,*R=34,O=2):...`, so
there are no visible `if` or `switch` statements for the different states.

10. Fixing the output after writing it

When the printer reaches a gap that would split a name or an operator like
`<<`, it backs up and writes `/*` over the characters it already printed, then
writes `*/` after the gap. At the end of a macro line it overwrites the last
character with `\`, and inside the string it overwrites it with `"`. Comments
that are too short become `;;;` instead.

11. Scrambled comments

The text that fills the rest of a line is `126&*++r`, a character of the source
with its lowest bit cleared. This turns `/` into `.`, so the comment can never
end too early, and makes the filler look like garbage.

12. Old style and misused types

`main(Z,Y)char**Y;` uses old K&R parameters. `char*U=Z` stores `argc` in a
pointer so `U-1` is true when there is a command line argument. `int**w=Y`
stores `argv` as the wrong type, and `isalnum` is used without a declaration.

13. Comments and line breaks that split tokens

`#define/**/F/***/for/*A*/` uses comments instead of spaces, and backslash
line continuations split words like `E\[1]` and `C\()` in the macros, which
hides the code even before the macros are expanded.
