# 2014/deak

Difficulty: Easy

1. Unary numbers from a variadic macro

Almost every number is written by counting. The macro `__(...)` puts its arguments
into an array, `(int[]){0, ...}`, then uses `sizeof` to get the number of elements
and subtracts one for the extra `0`. So `__(_,_,_)` is 3, because three things were
passed. The arguments are never used for their value, so they can be anything, like
`__('_','_')` or `__(',',',',',' ,',' ,',')`, which is 5. A trailing comma, as in
`__(_,_,)`, does not add an element, so that is 2. `simple.c` uses plain numbers.

2. Numbers built from arithmetic

Larger numbers are put together from the unary ones. The macros `_$()`, `__$()` and
`_$_()` are 0, 1 and 10, and `____` is `+ __`, so `_$_()____(_,_)` reads as 10 + 2,
which is 12. The maximum iteration count of 255 is written as 2 * 5 * 5 * 5 + 5,
using `__('_','_')*__('_','_', '_','_', '_')*...`. `simple.c` has `MAX_ITERATIONS`
and the other values as named constants.

3. Keywords hidden by token pasting

No C keyword appears in the source. The `##` operator glues pieces together while the
preprocessor runs, so `$(n,i,t)` becomes `int`, `$(ub,do,le)` becomes `double`,
`$(tch,pu,ar)` becomes `putchar`, `$_(f,i)` becomes `if` and `$_(to,go)` becomes
`goto`. The macro arguments are in a swapped order, so the pieces do not even read
correctly left to right. The `==` operator is pasted the same way with `$_(=,=)`.

4. Macros that hide punctuation

`r(e,t)` expands to the whole statement `return 0;`, `$_$(a)` adds a semicolon after
its argument and `$_$_` is `=`. The macro `m(i,a,u)` is the worst one because it
expands to `}; main`. The closing brace of the array and the name of the function are
in one macro, so the start of `main` is not visible at all. Later uses of it are
written after a lone `{` so that the hidden `}` closes it and the braces still match.

5. Digraphs

`<%`, `%>` and `<:`, `:>` are alternate spellings of `{`, `}`, `[` and `]`. They are
used for the array, for the body of `main` and for the character table, which removes
even more familiar looking C from the file.

6. Names made only of symbols

Nearly every name is built from `_` and `$`: `_`, `__`, `___`, `____`, `$`, `$_`,
`_$`, `$_$`, `_$_` and `$_$_`. They look alike and are easy to confuse. The `$_` name is
even both a macro and the parameter of `main`. It only works as a macro when
parentheses follow it, so `$_ $_(=,=) __(_)` uses both meanings side by side.

7. One array for all the variables

Every variable lives in the `double` array `_`. The bounds are `_[0]` to `_[3]`, the
squares are `_[4]` and `_[5]`, the point is `_[6]` and `_[7]`, `z` is `_[8]` and
`_[9]`, the sizes are `_[10]` and `_[11]`, the current row and column are `_[12]` and
`_[13]`, the iteration count is `_[14]`, the limits are `_[15]` and `_[16]`, and
`_[17]` is a flag that means the picture is done. With a number instead of a name, a
reader has to keep this whole table in mind. Since the indexes are also written in
unary, even the number is hidden. `simple.c` gives each one a name, and counters
become `int` variables.

8. Recursion through main instead of loops

There is no `for` or `while` in the program. `main` calls itself: `main(0)` moves to
the next column and `main(1)` moves to the next row, so `main` is both the column
loop and the row loop, and its parameter says which one it is being used as. Calls
never return until the very end, so the stack grows to thousands of calls. When the
last row is finished, all of them return and keep running the code after their calls,
so the `_[17]` flag is needed to stop them from printing more newlines. `simple.c`
has two nested `for` loops.

9. Loop written with goto

The iteration of `z` is made from the label `_f:` and `if (...) goto _f;`. That is
a `do`/`while` loop written by hand. `simple.c` has a real one, in its own function.

10. Character table written as numbers

The characters to print are not in a string. They are a `char` array named `imau`
with values like `__(_,_,_) * _$_()____(_,_)`, or 3 * 10 + 2, which is 32, the code
for a space. The array has six entries, `" +@-.*"`, but it is indexed with `% 5`, so
the last `*` can never be printed. `simple.c` has the string `" +@-."`.

11. Calling through a function pointer

`___` is a global pointer that is set to `putchar`, so a call like `___(10)` does not
look like printing. `simple.c` calls `putchar` directly, and `10` becomes `'\n'`.

12. Source code shaped by line breaks

Macro calls and expressions are broken up and spaced out to fill a neat block of
text, so a statement can start in the middle of one line and end on the next. Spaces
are even placed inside array accesses like `_ [ __(_,_,_,_) ]`. The only readable line
is the one with the bounds, which the author left alone on purpose.
