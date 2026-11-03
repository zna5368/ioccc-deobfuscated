# 1991/ant

Difficulty: Medium

1. Single-letter names for everything

Every global and function has a one-letter name: `b` is the text buffer,
`g` and `h` are the start and end of the gap, `c` is the end of the buffer,
`p` is the cursor position, `m` and `n` are the first and last positions
shown on screen, `d` is the number of screen rows and `q` is the quit flag.
The functions are named `Z`, `P`, `G`, `M`, `N`, `A`, `Y` and so on, so
`p=A(M(M(p)-1),x)` gives no hint that it means "move the cursor to the same
column on the previous line". Several names are also used twice: `j` is a
global and also a parameter of `A`, and `L`, `H`, `J`, `K`, `W`, `R`, `Q`
are function names that look like the editor keys but are not matched up
with them (`L` is "left", while the `L` key means "word right").

2. Macros that hide keywords and half-expressions

`#define V return` and `#define _ while` hide two keywords behind symbols.
`#define T isspace(*(t=Z(p)))&&` is only the first half of a condition, so
`_(!T b<t)--p;` really reads `while (!isspace(*(t = Z(p))) && b < t) --p;`.
Because the macro ends in `&&`, you cannot understand it without expanding
it in your head.

3. Shared global temporaries

The globals `i`, `j` and `t` are reused as scratch variables by almost
every function. `i` is a file descriptor in `main`, then an index into the
key table, then a column counter in `A`, then a row counter in `Y`. Since
they are globals, the reader has to check whether one function's value is
still needed after calling another. In the rewrite each function has its
own local variable.

4. Dispatch through a key string and a function pointer table

Commands are not handled with `if` or `switch`. The string
`k[]="hjklHJKL[]tbixWRQ"` is searched for the pressed key, and the index
found is used to call a function from `int(*z[])()={L,D,U,R,...,G}`. The
pairs only line up by counting positions in both lists. The table has one
extra entry, `G` (move the gap), which is called when the key is not found
because the search stops on the string's terminating `'\0'`.

5. Comparisons written as subtraction

Tests like `*t-'\n'`, `j-'\b'`, `p-n` and `m-P(c)` use subtraction in
place of `!=`: the result is zero (false) only when the two values are
equal. `p-n||(y=i,x=j)` therefore means "if `p == n` then set `y` and `x`".

6. `&&`, `||` and `?:` used as `if` statements

Short-circuit operators replace many `if` statements: `0<p&&--p` only
decrements when `p` is positive, `g-h&&(*g++=...)` only inserts when the
gap is not full, and `++i<d&&mvaddstr(...)` only prints `<< EOF >>` when
there is room. The comma operator packs several statements into one, as in
`addch(*t),j+=...` and `m=M(m-1),U()`.

7. Buffer gap arithmetic squeezed into one-liners

The editor keeps the file in one array with an empty "gap" at the cursor.
`Z` turns a file offset into a memory address and `P` does the reverse,
both by adding or removing the gap size `h-g` in a single expression such
as `V b+a+(b+a<g?0:h-g);`. Small tricks are hidden inside: `X` deletes a
character just by doing `++h` (making the gap one bigger), and `F` moves
the gap to the start of the file so that the whole text is one block it
can write with a single `write` call.

8. Tab stops with bit masks

`8-(i&7)` computes how many columns a tab advances: `i&7` is `i % 8` for a
non-negative number, so a tab moves to the next multiple of eight. It is
written inline twice instead of as a named helper.

9. Old K&R function definitions and implicit `int`

Functions such as `P(a)char*a;{...}` and `main(u,v)char**v;{...}` use the
pre-ANSI style where parameter types are listed after the parentheses, and
no function declares a return type. Most functions return nothing useful
but are still implicitly `int`, which is what lets them all be stored in
the same `int(*z[])()` table.

10. Compressed layout

The program is packed into a few dense lines with many functions on each
line and line breaks placed in the middle of expressions (for example a
break right after `_(T`), so the structure of the code cannot be seen from
its shape.
