# 1996/schweikh1

Difficulty: Medium

1. The whole program is made of macros

Apart from three `#include` lines, every line is a `#define`. The only line of
real code is `extern int errno;` and it is not even in `schweikh1.c` itself;
it is the last line of the file. The preprocessor turns that single line into
the entire program by expanding macro after macro. To read the program you
must expand every macro by hand (or run `cpp`) until only plain C is left.

2. `errno` and `extern` are redefined to hide `main`

`#define extern` makes the word `extern` disappear. `#define errno
translation_unit int errno` makes the word `errno` expand into the full
program followed by `int errno`. So `extern int errno;` becomes
`int main(void) { ... } int errno;`. `translation_unit` expands to
`declarator compound_statement`, `declarator` is `main ( void )`, and so on.

3. A fake `errno.h` in the entry directory

The Makefile uses `-I.` so the compiler finds the local `errno.h` before the
system one. The local file only contains `extern int errno;`, which is the line
that starts the whole expansion described above. With the real system
`errno.h`, `errno` is usually a macro of its own and the trick would break.

4. Macro names that pretend to be grammar terms

The macros are named after parts of the C grammar, such as
`translation_unit`, `compound_statement`, `declaration_list`,
`statement_list`, `iterative_statement`, `selection_statement` and
`relational_expression`. The names look like they explain the code, but most
of them do not hold what their names say. For example `true` and `false` are
the two branches that compute an April or March date, `keyword` sets the April
date, `tokens` holds the April 25 exception, `cast` calls `strftime` and
`puts`, and `syntax_error(n)` is `} else if (J < n) {`.

5. The source is written in reverse order

Each macro uses macros defined below it, so the file has to be read from the
bottom up. The lines are also indented into a triangle shape so the layout
looks like decoration rather than code.

6. Assignment hidden as a macro call

`ae(x, y)` (short for "assignment expression") expands to `x = y`. Every
assignment in the program is written as `ae(...)`, for example `ae(J,1582)`
and `ae(e,(2*b+4*c+6*d+N)%7)`. It even hides an initializer:
`struct tm ae(s,{ 0})` becomes `struct tm s = { 0 }`.

7. Unbalanced braces inside macros

`syntax_error(n)` contains `}` and `{` that only match up with braces from
other macros. `macro(x, y, cast)` uses it to build one link of the
`if (J < 1700) { ... } else if (J < 1800) { ...` chain that picks `M` and `N`
for each century. The `list` macro then closes the last branch with a lone
`}`. Looking at any single macro, the braces never balance.

8. Reused names for different things

The name `cast` is a macro (the `strftime`/`puts` call) but it is also the
name of a parameter of `macro(x, y, cast)`, where it means the year limit.
Similarly `a`, `b` and `c` are local variables of the program but also the
parameter names of `selection_statement(a,b,c)`. The same spelling means
different things depending on where it appears.

9. Obscure return value

`return'^'^'^'` is the character `'^'` XORed with itself, which is just
`return 0`.

10. Weekday that is never computed

The date is printed with `strftime` using `%a`, which prints the day of the
week from `tm_wday`. The program never sets `tm_wday`, but the `struct tm` is
filled with zeros, and zero means Sunday. Since Easter is always a Sunday the
output is correct without any weekday calculation.
