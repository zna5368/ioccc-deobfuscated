# 2004/sds

Difficulty: Medium

1. Whitespace between every token

Almost every keyword, name, number and operator in `sds.c` sits on its own line
and is surrounded by a random-looking mix of spaces, tabs and newlines. The
compiler ignores this whitespace, but it makes the program very hard for a
person to read and it defeats code formatting tools.

2. A second program hidden in the whitespace

The whitespace is not random. Every run of five whitespace characters is one
character of a hidden program: space, tab and newline stand for the digits 0, 1
and 2 of a base-3 number. Running the program on its own source (`./sds < sds.c`)
prints the hidden encoder. So the messy layout from technique 1 does double duty
as a data store.

3. Fake code inside comments

The comment that starts after `int main` contains what looks like real code: a
function `n`, a function `x` with its own variables and a `while` loop. Later
comments hold functions `o` and `y`. None of it is compiled, because it is all
between `/*` and `*/`. Since the comment markers are buried in whitespace,
a reader can easily miss them and try to understand code that does not exist.

4. Junk calculation in the helper function

The helper `m(c)` computes a long expression `x = 3 + (c - 1) * 9 + ...` but the
only place `x` is used is `x % x`, which is always 0. The whole calculation
is a decoy. What `m` really does is the nested `?:` chain at the end: it returns
1 for a space (32), 2 for a tab (9), 3 for a newline (10) and 0 for anything else.

5. Nested ternary operators

`c - 32 ? c - 9 ? c - 10 ? ... : 3 : 2 : 1` is a lookup table written as three
nested `?:` operators. `c - 32` is "true" (non-zero) when `c` is not 32, so the
chain falls through to the next test until a match is found. A `switch`
statement says the same thing far more clearly.

6. A loop that never runs

`for (; d < 0; d++) { ... }` looks like an important inner loop, but `d` was
just set to the character read by `getchar()`, which is never negative inside
the loop (the `EOF` case already ended the outer loop). The body, with its
shifts and divisions, is never executed.

7. A variable that always ends up zero

`f` is a global, so it starts at 0. In the main condition it is decremented
(`--f`) and then incremented again (`++f`), so it is back to 0 by the time it
is used. After that, `f / 243`, `+ f`, `- f` and `f == 1 ? ... : 1` add nothing
or always pick the harmless branch. Its only job is to make the expressions look
like they depend on some hidden state. The final `return (f && getchar());` is
therefore just `return 0;`.

8. Conditions that are always true

The `if` starts with `(v % v + f * --f + v)` and
`(v < f ? v / v - 992 * b : v * v)`. Because `v` is a positive character code
these are always non-zero, so they only exist to hide the `--f` and to make the
condition look complicated.

9. A chain of `&&` used as a sequence of statements

The real work is done inside one long `if (A && B && C && ...)`. Each part
assigns a variable from `getchar()` through `m()`. Because `&&` stops at the
first false (zero) value, the chain reads characters one at a time and gives up
as soon as one of them is not whitespace. This is an `if` used as a loop with an
early exit.

10. Disguised constants

Simple numbers are written as puzzles: `(993 - '!') / ('(' * 8)` is
`960 / 320 = 3`, `c - '9' + '8'` is `c - 1`, `243 / 9` is 27,
`3888 / '0'` is `3888 / 48 = 81`, and the scattered `- 4`, `+ 11` and `- 7` add
up to zero. Once decoded, the output is
`(a-1) + (b-1)*3 + (c-1)*9 + (d-1)*27 + (e-1)*81`, a five digit base-3 number
written out by hand instead of with a loop.

11. Chained assignment and reused single-letter globals

`(e = d = c = b = v = getchar()) - EOF` copies the same character into five
variables at once and compares with `EOF` by subtraction instead of `!=`. The
same short names `a`, `b`, `c`, `d`, `e` are then reused for the five base-3
digits, so it is hard to tell which value a name holds at any moment.
