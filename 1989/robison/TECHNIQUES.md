# 1989/robison

Difficulty: hard

1. Code generation with token-pasting macros

`#define _(D,E,F,G,H)` uses `D##f`, `D##g`, `D##h` and `D##i` to build five
functions from one line. For example `_(k,d,c(h),c(h),c(h))` creates `kf`, `kg`,
`kh`, `ki` and the dispatcher `k`. The 15 short `_(...)` lines become about 75
functions, so the reader has to expand the macros in their head to see the real
code. The original source (robison.orig.c) used the old pre-ANSI trick
`D/**/f`, where an empty comment glues two tokens together, instead of `##`.

2. Old-style (K&R) function definitions hidden in macros

`Y(_,A)` expands to `B _(d,e)B d,e;{return A;}` and `Z(P)` to
`C P(f,g,h,i)C f,g,h,i;{return P;}`. The parameter types are written after the
parameter list, which is legal in old C but hard to recognise. Every function
gets two parameters `d` and `e` even when it uses only one, and many calls pass
fewer or more arguments than the function takes.

3. Function pointers used as data tags (Church encoding)

Each list node stores one of the functions `f`, `g`, `h` or `i` in its `d` field
instead of a normal number or enum. Each of these functions takes four arguments
and returns the first, second, third or fourth one. So `(*d->d)(kf,kg,kh,ki)`
picks one of four handlers based on the node's tag. This takes the place of an
`if` or `switch` statement. simple.c uses an `enum` and `switch` instead.

4. No arithmetic, comparisons or control-flow keywords

The author's "C--" uses no `+`, `-`, `<`, `if`, `while` or `for`. All decisions
are made with the tag dispatch described above, and all loops are recursion.
The big-number arithmetic (shift, bitwise not, negate, increment, decrement,
add, multiply) is built from recursive functions over a linked list of bits.

5. Two's complement numbers as linked lists

Numbers are lists of bits, lowest bit first, ending in a node meaning "all zeros
from here" (`f`) or "all ones from here" (`g`). So `f` alone is 0 and `g` alone
is -1. Nothing in the code explains this, so the reader has to work out what
functions such as `m` (bitwise not), `p` (negate), `q` (+1), `r` (-1), `u`
(a + 2b), `v` (add) and `t` (multiply) do from their four cases.

6. Single-letter names that look alike

The functions `f`, `g`, `h`, `i` share their names with the parameters of `Z`,
and the parameters `d` and `e` share their names with the struct fields
`d` and `e`. The struct `A`, pointer type `B` and function pointer type `C`
also tell the reader nothing about what they are for.

7. Handlers reused for unrelated jobs

Since every function has the same signature, the generated functions are reused
for other purposes. `xf` (read a character and dispatch) is also the handler for
spaces, `kf` (return the argument unchanged) ends an expression on `)` and
newline, `mg()` makes the number zero, `mf` makes -1, and `zh`/`zi` shift a
binary digit into the number. The names say nothing about these roles.

8. Character dispatch table as the parser

`n[256]` maps input characters to handler functions, and `xf` calls
`(*n[getchar()])(d)`. Parsing is a chain of these calls, where each handler reads
the next character itself. Operators like `yi` (multiply) recursively parse the
whole rest of the line before applying the operator, which gives APL's
right-to-left grouping without any explicit precedence rules. Characters with no
handler leave a null pointer in the table, which is why `100 / 100` crashes.

9. printf used for argument evaluation order

`o("0",s a)` expands to `printf("0", s(d->e))`. The extra argument is ignored by
printf, but it must be evaluated before printf is called, so the higher bits get
printed first. The same trick is used in `p(d,o("-"))` to print the minus sign
before the digits, and in `main` where `printf("\n", zg(yf()))` evaluates and
prints the whole expression before the newline. The `int` returned by printf is
cast to a pointer `(B)` to fit the common function type.

10. Sign selected by calling a tag function with two arguments

`(*(*j(d)->d)(w,x))(d)` finds the end node of the number with `j` and calls its
tag function with only two arguments. `f` returns `w` (print a positive number)
and `g` returns `x` (print a negative number), so the sign check is hidden
inside a function call.

11. Misleading layout

The judges note that the lines can be put back in order with an ASCII sort.
Lines are arranged so that their first characters decide their position (a
leading space puts the `typedef` first, `#` lines come next, `_(` lines are in
alphabetical order). This puts the `typedef` before `#include` and the macro
definitions in an odd order, which hides the structure of the program.
