# 1995/leo

Difficulty: Hard

1. Chemical element names for identifiers

Almost every name is a symbol from the periodic table: `Ag`, `Au`, `Zn`, `Zr`,
`Mg`, `Ti`, `Ge`, `Li`, `Al`, `Sg`, `Mn`, ... The names say nothing about what
the code does. simple.c uses names such as `add_end`, `visited`, `diagonals`,
`cycles` and `frequency`.

2. Types and constants hidden in the Makefile

`Ga`, `As`, `Mo`, `Ar` and `S` are never defined in leo.c. The Makefile passes
`-DGa=608 -DAs=792 -DMo=int -DAr='unsigned long' -DS='struct Se'`, so `Ar *U`
is really `unsigned long *U` and `S { ... } *C` is really a structure. Reading
only the C file you cannot even tell the types. simple.c writes the types out
and names the page size `WIDTH` and `HEIGHT`.

3. The same name used as a macro and as a variable

A function-like macro is only expanded when its name is followed by `(`.
leo.c uses this so that `Li`, `Au`, `Ge`, `H`, `W` and `U` each mean two or
three different things. `Li(V,K)` is a macro that indexes a bitmap, but `Li/2`
is the frequency variable. `Au(P,0)` reads the direction table, but `Au` alone
is the diagonal bitmap. `Ge(P)` is a macro, while `Ge = Zr(...)` inside it is
a local pointer. The structure members `U` and `H` also share names with the
global `U` and the macro `H`. simple.c gives each thing its own name.

4. Member access hidden in a macro

`#define N O->` makes `N T`, `N H` and `N V` mean `O->T`, `O->H` and `O->V`.
Because the arrow is gone, it looks like two separate words next to each
other. simple.c writes the `->` out.

5. Bitmaps hidden behind macros

`Li`, `Mg` and `H` (bit index, test bit, set bit) turn two `unsigned long`
arrays into one bit per point on the page. `Mg(U, V, K)` asks "has an open end
already been at (V, K)?" and `H(Au, ...)` records "a diagonal line already
crosses this square". simple.c names them `BIT_WORD`, `TEST_BIT` and `SET_BIT`.

6. Direction table computed by a formula

The eight compass steps are not written out. A loop fills `Al` with
`!!(V&3)*(1-(V&4)/2)*4`, which gives `0,4,4,4,0,-4,-4,-4` for the x step. The
same value is also stored at `Al[8|(V-2)&7]`, which shifts it by two places
to give the y step in `Al[8..15]`. The macro `Au(P,K)` then picks
`Al[K | (dir+P+P-1)&7]`, where `K` is 0 for x and 8 for y. simple.c writes
two plain arrays `dx[8]` and `dy[8]`.

7. Writing through a chosen pointer

`*(Mg(U,V,K)?&W:&O)=0;` sets either `W` or `O` to zero depending on a test,
and `*(H ? &O : &Zn) = O = *(O ? &N H : &C) = U(sizeof(S));` inserts a node
into the linked list in a single chained assignment: it links the node in
after `O` (or at the front), then updates the tail if needed. simple.c uses
normal `if` statements.

8. A whole search packed into a loop condition

`Zr` walks the list with
`O = (W = !N T || (N T &= -!!(...)) ? O : 0) ? N H : 0;`. The `&= -!!x`
trick turns the result of a test into a mask: `-!!x` is all ones when `x` is
not zero and zero otherwise, so the step is either kept or cleared. Clearing
the step both cancels the node and stops the loop. simple.c splits this into
`cancel_meeting`, a plain `for` loop with an `if`.

9. Argument parsing inside a `while` condition

The loop body is empty. A number is read with `sscanf` into `W` the first time
and into `Li` after that (`W ? &Li : &W`). Any other word has its first
letter decremented, and `!((c-57)%14) << (c-57)/14` turns it into a flag bit.
Only letters 58 + 14 * n (`:`, `H`, `V`, `d`, `r`, and byte 0x80) give a bit,
which hides the secret switches. simple.c uses a `for` style loop with named
flags (`NO_LOOP`, `HAIRY`, `VARIABLE`, `DEEP`, `RIGHT`, `NO_VISITED`).

10. Borrowing a buffer element as a loop counter

`for (*U = -W-1, H = U(W); ++*U; H[*U+W] = Mn);` uses the first word of the
freshly zeroed bitmap as its counter. It starts at `-W-1` (a huge unsigned
number), counts up to zero, and leaves the word back at zero when done.
simple.c uses a normal counter `i`.

11. Numbers written as expressions

`W((!!C>>!C))` and `W((!!C<<!!C))` are really `W(1)` and `W(2)`, because
`C` is never NULL there: `!!C` is 1 and `!C` is 0. The direction of a new end
is `(N U + 2*P - 3) & 7` and the branch is drawn only if `4-2*P-D[8]` is not
zero. simple.c names the left and right branches and spells out these cases.

12. Branch choice and order hidden in arithmetic

The two `Ge(...)` calls get arguments like
`1 + D[8]/2 + ((1 - h) & D[8])` and `(1 + h) & -(D[8]&1)`. Working through
the values shows they mean "grow only the left branch", "grow only the right
branch", or "grow both, in an order that depends on the deep and right
flags". A zero argument makes `Ge` do nothing. simple.c computes `left_first`
and uses three `if` statements.

13. Counting true comparisons

`Mn` is `(V>=Li/2)+(V<Li)-(V<Li/2)`. Each comparison is 0 or 1, so the sum is
0, 1 or 2 depending on which range the random number falls in. simple.c
writes it as `random_branching` with two `if` statements.

14. `&&`, `||` and the comma operator instead of `if`

`F & 32 || H(U, V, K)` only sets the bit when flag 32 is off. The `W` macro is
one long `&&` chain that ends with `printf`, the `Ti` macro hides a `?:` with
comma expressions inside, and `Ge` ends with `(Ag(...), 0)` so the call is
made but the whole expression is still false. `free((C = N H, O))` moves the
head of the list and frees the old one in one argument. simple.c uses
ordinary statements.

15. Short PostScript names

The PostScript header defines `S`, `N` and `O` as `0 setlinewidth`,
`showpage` and `moveto rlineto stroke`. The three `def`s are written together
at the end, so they are applied from the last pair to the first. Each line
the program prints, like `-4 4 304 400 O`, is a full drawing command.
simple.c keeps the header but describes the commands in comments.
