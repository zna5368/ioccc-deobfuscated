# 1992/marangon

Difficulty: Hard

1. Short macro names hiding library calls and keywords

`#define I for`, `#define Q return`, `#define ST strchr`, `#define RD lrand48`,
`#define K endwin()` and `#define W wrefresh(r)` replace keywords and library
functions with one or two letters. `typedef int i;` makes `i` mean `int`, which
also looks like a loop counter. The reader has to keep a table of the
substitutions in mind to read any line.

2. Meaningless single-letter names

The board is `z`, the window is `r`, the help text is `g`, the cursor row and
column are `a` and `b`, the key is `c`, and the functions are `nt` (setup),
`ci` (draw the board) and `d` (change a cell). `z` is also indexed as
`z[column][row]`, the opposite of what most people expect.

3. The whole game loop squeezed into a single `for` statement

The main loop is one `for` whose initialisation draws the board, whose
condition reads a key and does all of the key handling, whose "increment" part
checks for a win and moves the cursor, and whose body counts how many cells are
still `5`. The body is itself two nested `for` loops with empty bodies. Nothing
happens in a normal `{ ... }` block.

4. Key handling with bit tests instead of comparisons

Keys are never compared with `'h'` or `'+'`. Instead the code tests single bits
of the character code: `c&64` is true for the letters `Q h j k l` and false for
`+ -`, `c&16` is only true for `Q`, `!(c&7)` only for `h`, `c&4` picks `l`, and
`c&2 && ~c&1` picks `j`. `strchr("+-Qlkjh", c)` first throws away any key not in
the list. The `+`/`-` choice is passed on as `~c` and tested with `&2`.

5. Control flow written as `&&`, `||` and `?:` chains

Every `if` is written as a boolean expression. For example
`--b<0 && (b=q^s)&0` means "move left, and if you fall off the edge wrap to
column 5". The `&0` makes the result always false so that the `||` chain moves
on to try the next key. `K,ext(1)` (with `#define ext(a) (exit(a),0)`) is an
`if` statement written with the comma operator, and the win test
`sc==36 ? ci(),W,K,ext(0) : wmove(...),W` is an `if/else` written with `?:`.

6. Hidden constants built from a magic table

`f[3][3]` is an orthogonal-looking matrix whose every column has a sum of
squares of `4225`. The loop `s += f[q][t]*f[q++][t]` picks a random column
just to always get `s = 4225, q = 3`. The loop condition then computes
`q ^= s %= 15` (giving `q = 9, s = 10`), `s /= 2` (5) and `s ^= q` (12). The
"random" looking values `q ^ s = 5` and `++q/2+1 = 6` are then used as the
wrap-around column and the board size.

7. Wrap-around code disguised as nonsense arithmetic

When the cursor goes past the right or bottom edge it must go back to `0`.
This is written as `c&4 < (~b&23 ? q^=s, b^=b : ...)` and
`c&16 > (!a|12 ? a=7, a&=~a : ...)`. Because `<` and `>` bind tighter than `&`,
these are really `c & (4 < 0)` and `c & (16 > 0)`, and the only useful parts
are `b^=b` and `a=7, a&=~a`, which both set the variable to `0`. The other
`?:` branches can never be reached.

8. Undefined behaviour that changes with the compiler

`(q ^= s %= Y(q), s /= 2) != (s ^= q)` changes `s` on both sides of `!=`
without a sequence point. With clang the left side runs first and the test is
true (5 != 12), so the cursor keys work. With gcc the right side runs first and
the test ends up comparing `5 != 5`, so the cursor can never move and only the
top-left cell can be changed. `simple.c` implements the intended (clang)
behaviour. The same kind of trick decides the order in which the three random
numbers in `d(Z(0,5),Z(0,5),Z(1,2))` are drawn, because C does not say in
which order function arguments are evaluated.

9. Function returning a function pointer

`d` is declared `i (*d())()` and returns `ci`, the draw function. `main` stores
`d` in `i(*(*p)())() = (nt(),d);` (a comma expression that also runs the setup
function) and calls `(**p(b,a,~c))()`, which changes the cell and then calls the
returned pointer to redraw the board. Its return value of `0` keeps the `||`
chain going.

10. Loop conditions that do the work

In `d`, the second pair of loops has an empty body; the cells are changed inside
the loop condition `(x==a&&y==b&&(z[x][y]+=...)) || (z[x][y]+=...)`. The win
check `for(sc=0; sc<36 && *((char *)z+sc++)==5; );` also hides its work in the
condition and treats the 2D board as a flat array. Because `sc++` happens even
when the comparison fails, it reaches `36` when only the first 35 cells are `5`,
so the last cell is never checked.

11. Old-style (K&R) declarations and implicit types

`nt()` has no return type, `d` uses the old `i (*d(a,b,c))() i a,b,c;` parameter
syntax, `time`, `strchr` are declared by hand, and `exit`/`memset` are used with
no header at all. This avoids `#include` lines and makes the declarations hard
to read.

12. Macros for screen layout and random ranges

`X(x)` and `Y(x)` turn a board row/column into a window position
(`2*x+1`, `4*x+3`) and are reused for the window size (`X(6)`, `Y(6)`) and even
for the magic number `Y(q)` in technique 6. `Z(d,a)` gives a random number from
`d` to `a`.
