# 1989/jar.2

Difficulty: Hard

1. Abbreviated preprocessor directives

The first line of `jar.2.orig.c` is `#define d define`, and then the author
writes `#d a include` and `#a <stdio.h>`, hoping `#d` and `#a` will be read as
`#define` and `#include`. Old preprocessors allowed this, but in standard C the
word after `#` is never replaced by a macro, so `jar.2.c` had to write the
directives out in full. The two macros are still there, but they do nothing.

2. Single letter macros that hide keywords and library calls

`U` is `return`, `W` is `while`, `p` is `char*`, `z` is `atof`, `e` is
`isspace` and `X` is `sbrk(199)`. `T(E)` means "is this the empty list `()`".
Every line has to be translated in your head before you can read it. `simple.c`
writes the real words, and uses `new_string()` and `is_nil()` for `X` and `T`.

3. Macros that write whole functions

`B(y)` expands to the start of a function definition, `p y(_)p _;{`, so
`B(car)` would begin `char *car(_) char *_; {`. `G(y,V)` builds on it to make a
complete function that evaluates two arguments and combines them with the
operator `V`, so `G(f,+)G(g,-)G(h,*)` creates the add, subtract and multiply
functions in one short line. Because the function names are `f`, `g`, `h`, `c`,
`q`, `t` and so on, you only find out what they do by looking at the table that
pairs them with names like `"+"` and `"car"`. `simple.c` writes each function
out with a name such as `builtin_add`.

4. Macros that hide common expressions

`D` is `A(_)` (the rest of the argument list) and `E` is `S(C(_))` (evaluate the
first argument). So `S(C(D))` means "evaluate the second argument". These are
used everywhere and make the code look much simpler than it is. `simple.c` has
`first_arg()` and `second_arg()` instead.

5. Meaningless names

The functions that do the real work are `Q` (length of the first element), `C`
(first element, like car), `A` (rest of the list, like cdr) and `S` (evaluate).
Every function's parameter is called `_`, the symbol table is split between
arrays named `r` and `not`, and there are locals named `Li` and `s` next to a
function named `L`. `simple.c` uses `element_length`, `first`, `rest`, `eval`
and `table`.

6. Everything is a string

There are no cons cells. A list is stored as its printed text, so `C` and `A`
take lists apart by counting parentheses with `Q`, and `cons` (function `t`)
glues text together with `sprintf`. Numbers are text too and are converted with
`atof` each time they are used, and results are written back with `"%lf"`. Even
functions are "strings": built-ins are stored as function pointers cast to
`char*`, and user functions are the text `((params) body)`. `S` tells them apart
by checking whether the first byte is `(`. For a built-in it then turns the
`char*` back into a function pointer with `(*((p(*)())u))(_)` and calls it.

7. One table written as two arrays

The symbol table is meant to be one array of name/value pairs, but it is written
as `r[4][2]`, holding the four special forms (`function`, `quote`, `lambda`,
`defun`), followed later by `not[99][2]` holding the built-in functions. The
code only ever uses `r`, with indexes far past its size of 4 (`M` starts at
14), and relies on the compiler placing `not` directly after `r` in memory.
Nothing in C promises that. With modern compilers at `-O3`, as the Makefile
builds it, `not` is placed before `r` and the program crashes before printing
anything (it works at `-O0`). `simple.c` uses one array, `table`, so its
output was checked against the original built with `-O0`.

8. Global variables reused as scratch space

`H`, `O`, `Y` and `K` are globals used for different things in different
places. `H` is the end pointer in `Q`, the second argument in `cons` and the
write position in `main`. `Y` is the element length in `C`, a loop counter in
`S` and the parenthesis depth in `main`. Because they are global, it looks as
if values are passed between functions through them, but they are never
needed across a call. `simple.c` uses local variables instead.

9. Comma operators instead of statements

Whole function bodies are squeezed into one `return` with commas, for example
`U _++,Y=Q(_),_=strncpy(X,_,Y),_[Y]=0,_;` in `C`, which steps past `(`, measures
the first element, copies it, ends the copy with a zero byte and returns it.
The `?:` operator is used as an `if`, as in `K==EOF?exit(0):0`. `simple.c`
splits these into ordinary statements.

10. Bitwise `|` instead of logical `||`

`W(Y|!e(K=getchar()))` in `main` uses `|`, which always evaluates both sides,
so `getchar()` is called on every loop even while inside parentheses. With
`||` the read would be skipped whenever `Y` is not zero. `simple.c` calls
`getchar()` first and then tests the condition.

11. Recursive `main` and dead code

Instead of a loop, `main` reads and evaluates one expression and then calls
`main()` again. The only way out is `exit(0)` at end of file, so the
`printf("XLISP 4.0\n")` written after the recursive call can never run. It is
only there to look like a startup banner. `simple.c` uses a `for (;;)` loop and
leaves the banner out.

12. A short way to pick the `if` branch

`S(C(A(T(E)?D:_)))` in `F` evaluates the condition, then picks either the
argument list or its rest, and then takes the rest again. When the condition is
false, the list has been moved forward twice so its first element is the else
part. When it is true, it has been moved once so the first element is the then
part. `simple.c` writes this as an `if` that skips the then part when the
condition is `()`.

13. Binding arguments with blank names

When a user function is called, `S` first stores each evaluated argument in a
new table entry named `""`, so no lookup can find it yet, and only afterwards
walks the parameter list and gives those entries their names. Resetting `M`
afterwards removes all of them at once. `simple.c` keeps this method and
explains it in a comment.

14. Old style C and hand-written declarations

Functions use K&R parameter declarations (`Q(_)p _;{`). `K`, `Y`, `M`, `Q` and
`L` have no type at all, so they are `int` by default. `sbrk` and `atof` are
declared by hand instead of through their headers, and `sbrk(199)` is used as
the memory allocator, which is never freed. `simple.c` uses modern function
definitions with types, and wraps the `sbrk` call in `new_string()`.

15. Shaped source layout

The code is spread out into a decorative shape with random spaces and tabs in
the middle of expressions, for example between `T(H` and `)?"(%s)"`, so the
lines don't match the structure of the program.
