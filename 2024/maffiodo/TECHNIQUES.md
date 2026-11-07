# 2024/maffiodo

Difficulty: Hard

1. One-letter names for everything

Every function, variable and struct field has a one- or two-letter name: `E`
allocates a value, `F` finds a property, `G` declares one, `U` evaluates an
expression, `R` runs statements, and fields like `m`, `x`, `b`, `q` hold the type,
the value, the next pointer and the property list. Some names are used more than
once, such as `struct d` with a field `e` of type `e`, and the field `x` next to
`#define z x o`. The rewrite uses names like `new_value`, `find_property`,
`evaluate`, `execute`, `type`, `value`, `next` and `props`.

2. Short macros for common code

`#define o ->`, `#define p if(`, `#define t f++`, `#define s ( )`,
`#define z x o` and `#define c sprintf` make the code look like random letters.
For example `p *f==61){ W=2; t; }` really means `if (*f == '=') { W = 2; f++; }`.

3. Macros that hide a switch statement

`#define I ; } else p i==j++){` together with the opener `p j=0){` turns a list
of `I` blocks into `if (j=0) {} else if (i==j++) {...} else if (i==j++) {...}`.
`j=0` is an assignment that is always false. After that, each `i==j++` compares `i`
to 0, 1, 2, ... in turn, so the code behaves like `switch (i) { case 0: ... }`.
The `a(x)` macro does the same thing for statement keywords, and uses `#x` to turn
`if`, `else`, `while`, `return` and `break` into the strings passed to `b()`.
The rewrite uses real `switch` statements and `if`/`else if` chains.

4. Operators selected by their position in a string

`K(",=!&|><-+*/%[(.")` returns the position of the current character in that
string. The position is used as the operator number and as its precedence, so
`i<h` decides when an expression stops and `i<13` decides whether to read a
right-hand side. `K("-+!~")` works the same way for unary operators. The rewrite
gives these positions names with an `enum` (`OP_COMMA` ... `OP_DOT`).

5. Character codes written as numbers

The parser compares characters with numbers instead of character literals:
`47` is `/`, `10` is newline, `40` is `(`, `91` is `[`, `123` and `125` are `{` and `}`,
`34` and `39` are the quote characters, `92` is a backslash, `110` is `n`,
`120` is `x`, `61` is `=`, `41` is `)` and `95` is `_`.

6. Library functions behind function pointer aliases

`int (*w)()=strlen, (*A)()=strcmp; char *(*D)()=strdup;` lets the program write
`w`, `A` and `D` instead of the real names, so you cannot tell what a call does
just by reading it. The rewrite calls `strlen`, `strcmp` and `strdup` directly.

7. Header files hidden in the Makefile

`prog.c` has no `#include` lines. The Makefile adds them with
`-include stdio.h -include string.h -include stdlib.h -include ctype.h`. Other
missing pieces also rely on old C rules: functions with no return type become
`int`, `main s` means `main()`, and many `int` functions never return a value.

8. One struct used for many purposes

The same `struct d` is used for numbers, strings, functions, objects, variables,
object properties, and the nodes of every linked list. A function is just a
string (type 2) that holds its own `{ ... }` source code. Which fields matter
depends on how a node is used, so the reader has to work out the role of each
node from the code around it. The rewrite comments every field and adds `TYPE_*`
constants.

9. Execution flags in a single global character

The global `r` combines three bit flags: 1 means `return` ran, 2 means `break` ran,
and 4 means the code is being skipped (for example an `if` whose condition was
false). Expressions like `r=u?d&~2:d|(r&3)` and `r=(!P&!d)*4` set and restore
these bits. The rewrite names them `FLAG_RETURN`, `FLAG_BREAK` and `FLAG_SKIP`.

10. Gotos into the middle of other branches

The comparison code is reached with `goto V` from the `!`, `>` and `<` cases. In
that code `goto O` and `goto Q` jump into the `else` branches below. The `[`
operator uses `goto q` to jump inside the `else` branch of the `.` operator. In
`R`, `while` does `goto u` into the `if` code, and the loop repeats with `goto j`.
In `U`, `goto g` re-reads the name after `var`. The rewrite replaces each goto with
a helper function (`compare`, `member`), a flag, or a normal loop.

11. Variable shadowing

Inner blocks declare new variables with the same names as outer ones, so a name
means different things a few lines apart. In `U` the parameter `h` (minimum
precedence) is hidden by `int ... h=0` (the "operator is doubled" flag). In `R`
the outer `d` (saved scope) and `u` are hidden by `int u=0,d;` (loop flag and saved
flags), and `char P` in the string reader hides `int P`.

12. Global buffer reused for unrelated work

The 99 byte buffer `g` holds the current identifier, the name to look up with
`F`/`G`, the text of a number converted by `Q`, a string literal while it is read,
and the key for `x[y]`. Because `Q` writes into it, every call can silently
overwrite the last name that was read.

13. Formatting that hides the structure

The code is packed into a dense block with no line breaks between statements.
The macros at the top are indented strangely, and there are large blank areas on
the left side. Even after preprocessing you need a formatter to see where any
function starts and ends.

14. Dead code

The garbage collector `_()` and its mark function `h()` are defined but never
called. The lines `z l=1; z l=0;` at the end of `R` mark a value as "retained"
and then clear it right away; a GC call was meant to go between them. The
variable `u=v` in `R`, which saves the heap position for that call, is also never
used. The rewrite keeps these pieces but comments that they never run.
