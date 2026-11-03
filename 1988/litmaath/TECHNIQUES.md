# 1988/litmaath

Difficulty: Hard

1. Whole program inside a `while` condition

The `while` loop has an empty body (`;`). Every action of the program,
including sorting, printing and moving between arguments, happens as a side
effect while the condition is being evaluated. The loop keeps going as long as
the condition is true, and the condition is arranged so that it only becomes
false when the last argument has been finished.

2. Short-circuit operators used as `if`/`else`

`&&` only evaluates its right side when the left side is true, and `||` only
evaluates its right side when the left side is false. The program chains these
to make decisions: `(++argc)[*argv] && (... ) && --argv || putchar(**argv) && ++*argv--`
means "if there is another character, compare it and step back, otherwise
print the first character and move on". `... || putchar(10)` means "if the
word is finished, print a newline".

3. A subexpression that is always false

`*argv != argv[1] && (*argv = argv[1]) && (argc = 0)` ends with
`(argc = 0)`, which has the value `0`, so this whole part is always false and
the `||` always moves on to the second half. It exists only for its side
effects: when a new starting position is detected it saves that position and
resets the counter to `0`.

4. Only two variables, reused for other jobs

There are no local variables. `argc` stops being the argument count after the
first pass and is reused as the index `j` of the character being compared.
`argv` is moved back and forth (`*++argv`, `--argv`, `++*argv--`) so that
`argv[0]` holds the saved start of the unsorted part and `argv[1]` holds the
current position. The program also overwrites entries of the `argv` array
itself, including the program name, to store this state.

5. Reversed array indexing

`argc[*argv]` and `(++argc)[*argv]` look strange, but in C `a[b]` is the same
as `*(a + b)`, which is the same as `b[a]`. So `argc[*argv]` is just
`(*argv)[argc]`, the character at index `argc` of the current string.

6. Swap without a temporary variable

`**argv += argc[*argv] -= **argv = argc[*argv] - **argv` swaps two characters
`a` and `b` using arithmetic: first `a = b - a`, then `b = b - a` (which
gives the old `a`), then `a = a + b` (which gives the old `b`). It also
modifies the same objects several times in one expression, which is not
allowed by the C standard, but it happens to work with common compilers.

7. Selection sort hidden by pointer stepping

The sort compares the first character with each following one and swaps when
the first is larger, so after one pass the smallest character is at the front.
Instead of keeping an outer loop counter, the program prints that character
and increments the string pointer itself (`++*argv`), so the "front" of the
string moves forward by one each pass.

8. Numbers instead of character literals

`putchar(10)` prints a newline; `10` is the ASCII code of `'\n'`.

9. Old style function definition and implicit declarations

`main(argc, argv) int argc; char **argv;` declares the parameter types after
the parentheses, the pre-ANSI way. `main` has no return type and returns no
value, and `putchar` is used without including `<stdio.h>`, both relying on
old C rules that assume `int`.
