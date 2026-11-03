# 1993/vanb

Difficulty: Medium

1. Whole program is one expression

The function `O5` is a single `return` statement made of a long chain of
nested `?:` conditional operators. There are no `if`, `while`, `switch` or
`for` statements at all, so every decision is hidden inside a ternary and every
loop is done by recursion. In simple.c the chain is split into one function per
grammar rule (`parse_expr`, `parse_term`, `parse_factor`, `parse_octal`, ...)
with ordinary loops and `switch` statements.

2. Recursion through a single function with a state number

All recursive descent parser functions are merged into `O5` (which `main`
calls). The first parameter `O2` is a "state" number that selects which
grammar rule to run: 3 = octal number, 4 = decimal, 5 = hex, 6 = factor,
7 = rest of a term, 8 = term, 9 = rest of an expression, 10 = expression,
11 = closing parenthesis. `main` itself starts with `O2 = argc`, so with one
argument (`argc == 2`) it lands in the "print the result" state, and with no
arguments (`argc == 1`) it just returns 0.

3. State selection by repeated decrement

Instead of comparing `O2` with each state number, every branch subtracts a
small amount from `O2` and tests whether it became zero: `!(O2-=02>01)`,
`!--O2`, `!(O2+=01-02)` and so on. Since each step removes 1, the branch
that hits zero is the one whose position in the chain equals the original
state number. You have to count the branches to know which rule is which.

4. Disguised constants

The value 1 (or -1) used in the decrements is written in many different ways
so that each looks unique: `~01+01` (-1), `02>01` (1, a comparison is true),
`02>>01`, `-O3-~O3` (always 1 because `~x == -x-1`), `!O3+!!O3` (always 1,
one of the two is true), `02-01`, `-02/02`, and `0563&0215` (the bitwise AND of
those two values is 1).

5. Everything in octal

Every number in the program is an octal literal: character tests such as
`**O7<=067` (`'7'`), `060` (`'0'`), `050` (`'('`), `0170` (`'x'`), multipliers
such as `010` (8), `012` (10) and `020` (16), and even the format string
`"\045\157\012"`, which is `"%o\n"` spelled with octal escape sequences.
simple.c uses character literals and decimal numbers.

6. Confusing identifiers

The only names are `O2`, `O3`, `O5` and `O7` (capital letter O followed by a
digit). Next to the many octal constants like `02`, `03`, `05`, `07` (zero
followed by a digit) they are almost impossible to tell apart. `O5` is just
`main` renamed by a `#define` in the author's notes, and the vanb.c version
has a second function `O5` that `main` forwards to. In simple.c they are
`argc`/state, `argv`/cursor and the accumulated `value`.

7. Many ways to compare or advance

Each simple task is written differently every time. Character equality is
tested with `==`, with subtraction `!(045-**O7)`, or with XOR
`!(**O7^057)`. Ranges use both `a<x && x<b` and `x<=b && a<=x` in mixed
order. Moving to the next character is done as `++*O7`, `(*O7)++`,
`*++*O7`, `*(*O7)++`, often wrapped in a useless calculation such as
`050**++*O7` or `03-*++*O7` whose value is thrown away by the comma operator.

8. Tricky token spacing

Expressions such as `*(*O7)+++O3*020-060` rely on the compiler's "longest
match" rule: it is read as `*(*O7)++ + O3*020 - 060`, i.e. take the current
character, advance, then add. Likewise `050**++*O7` is `050 * *++*O7`.

9. Accumulator passed as a parameter

The number parsers (`O5` in states 3, 4 and 5) do not use a loop variable;
the value built so far is passed back in as the third argument `O3` on every
recursive call (`O5(03,O7,*(*O7)++-060+010*O3)`), and the same trick is used
for the running total of `+ -` and `* / %`. simple.c turns these tail calls
into `while`/`for` loops.

10. Hidden pointer manipulation

The parse position is not a variable either. `O7` is `argv`, and the parser
advances by incrementing `*O7` (the pointer stored in the argv array), so
`**O7` is always the next character. Before the first call `main` passes
`O7+01`, meaning `&argv[1]`, so the expression argument is what gets parsed.

11. Program shaped like a picture

vanb.c is laid out in the shape of the Roman numeral VIII (eight, for octal).
The line breaks fall in the middle of tokens and expressions, so the layout
gives no hint about the structure of the code.

12. Old-style K&R declarations

`O5(O2,O7,O3)char**O7;` declares a function with implicit `int` return type and
implicit `int` parameters, using the pre-ANSI style where parameter types are
listed after the parentheses. `main` is also declared with a bogus third
parameter `char**O3` that is never used.
