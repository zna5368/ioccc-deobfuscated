# 2004/arachnid

Difficulty: Medium

1. Source code shaped as ASCII art (the maze itself)

The source is laid out as a maze picture and is the default maze file the
program loads when run with no arguments. Code is packed into "wall" blocks
and padded with runs of empty statements (`;;;`), empty comments (`/**/`,
`/******/`) and split string literals (`"q" "q" "k"`) so that every row
lines up. The empty statements and comments do nothing, and adjacent string
literals are simply joined by the compiler into one string.

2. Macros defined on the compiler command line

The Makefile passes `-DL=LINES -DS=COLS` so the single letters `L` and `S`
are really the ncurses screen height and width. `-DW=mvwaddch(w,v,u,acs_map[l[m[u`
is an unbalanced fragment of a function call; the source finishes it with
code such as `W][v-1]&15]])`, so the brackets in the source never seem to
match until the macro is expanded. Because `S` means `COLS`, the declaration
`int S,s;` inside `d()` actually declares a local variable called `COLS`.

3. Encoded output string

The final message is stored in `Q[]` as `` "Zpt!ftd`qdc!`eudq!$c!nnwft\040\t" ``.
The recursive function `cu()` walks to the end of the string and, on the way
back, adds or subtracts one from each character depending on whether the
character after it was odd or even. The result is
`"You escaped after %d moves!\n"`, which is then passed to `printf`.

4. Bit flags packed in one integer per cell

Each element of `m[256][256]` stores several facts as bits: 16 is a wall,
32 means the cell has been seen, 64 is the exit, and the low four bits
(1 right, 2 down, 4 left, 8 up) record which neighbours are visible walls.
Only the bare numbers `15`, `16`, `32`, `48` and `64` appear in the code.
simple.c gives each bit a name such as `WALL` or `SEEN`.

5. Lookup string for line-drawing characters

`l = "~qxlqqkwxmxtjvun"` is indexed by the four neighbour bits to choose a
letter, and `acs_map[letter]` turns that letter into an ncurses line-drawing
character (for example `q` is a horizontal line and `l` a top-left corner).
This lets walls be drawn as connected boxes without any `if` statements.

6. Ternary and comma operators instead of if statements

Almost every decision is written as `cond ? a, b : 0` or
`c==119&&!(m[x][y-1]&16)?y--:0`. The comma operator lets several actions run
inside one branch of `?:`, and the `:0` branch is a do-nothing value. In
simple.c these are ordinary `if`/`else` blocks.

7. Characters written as numbers

Keys and glyphs are written as numbers in different bases: `0163` (octal)
is `'s'`, `119` is `'w'`, `97` is `'a'`, `100` is `'d'`, `3-1+1` is Ctrl-C,
`64` is `'@'`, `60` is `'<'`, `46` is `'.'`, `32` is a space and `10` is a
newline. The `+1` in `(c=fgetc(f))+1` is just a hidden test for `EOF` (-1).

8. Dummy function used to chain calls

`Z(int a,int b){}` does nothing. The setup calls are hidden as its
arguments: `Z(Z(raw(),Z(curs_set(0),Z(1,noecho()))),keypad(stdscr,TRUE))`.
The calls happen only because arguments must be evaluated before a function
is called; the order they run in is left up to the compiler.

9. Confusing and reused names

Functions and variables have one-letter names (`c`, `d`, `e`, `cu`, `m`, `a`,
`b`, `v`). `v` is a coordinate in `c()` and `d()` but the move counter in
`main()`. `e(int y,int x)` is called as `e(x,y)`, so inside it the parameter
named `y` is really the x coordinate. The parameters `a` and `b` of `Z()`
hide the globals of the same name. `d()` is a Bresenham line-drawing routine
that casts sight lines, with its variables named `X`, `Y`, `S`, `s` and `f`.

10. Misleading redundant work

In `c()` every neighbour's glyph is drawn with the macro `W`, but the macro
always draws at position `(v,u)`, so each of those drawings lands on the
current cell and is immediately overwritten by the last one. Only the final
`mvwaddch` call has any visible effect.
