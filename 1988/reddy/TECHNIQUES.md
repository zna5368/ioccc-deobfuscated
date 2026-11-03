# 1988/reddy

Difficulty: Medium

1. Macros that hide keywords and library calls

`#define w printf` and `#define p while` make the code look like it calls
unknown functions `w(...)` and `p(...)`. The macro `#define t(s) (W=T(s))`
hides the fact that every call to the tokenizer also stores its result in the
global `W`. In `simple.c` these are written out as `printf`, `while` and
`next_token(...)`, which updates the global `token` itself.

2. One-letter global variables

All state lives in globals with meaningless names: `B` is the read position
in the line, `X` and `V` are the start and length of the current word, `W` is
the current token, `I` is the input buffer and `L`/`M` remember the base type
name. Because they are globals, the functions `D()` and `T()` talk to each
other without passing arguments, so you have to read every function to know
what a variable means. They are renamed `cursor`, `token_text`, `token_len`,
`token`, `line`, `type_name` and `type_len`.

3. Magic numbers instead of character constants

Characters are compared by their ASCII codes: `40` is `'('`, `41` is `')'`,
`42` is `'*'`, `91` is `'['` and `93` is `']'`. The tokenizer also reuses the
codes `9` (tab) and `32` (space) as private labels for "identifier" and
"number". This is safe only because tabs and spaces are skipped before a token
is read, so they can never be real tokens. The same `32` lets the test
`W<33` mean "the token is a word or a number". `simple.c` uses character
literals and the names `TOKEN_IDENT` and `TOKEN_NUMBER`.

4. Ternary and comma operators instead of `if` statements

`D()` is written as `W==9?(...):W==40?(...):W==42?(...):0;`, a chain of
`?:` where every branch is a list of actions joined with commas. The trailing
`:0` is a dummy value that is never used. The suffix loop works the same way:
`p(W==40?(...):W==91?(...):0);` is a `while` with an empty body, and all of
its work is done inside the condition. `simple.c` turns these into
`if`/`else if` blocks and a plain `while (token == '(' || token == '[')`
loop.

5. Work hidden inside loop conditions

`main` reads input with `p(w("input: "),B=gets(I))`. The comma operator
prints the prompt and then reads a line, all inside the `while` condition, so
the loop has no visible body for the prompt. The single `if` that follows
does a whole declaration with another comma list:
`L=X,M=V,t(0),D(),w(...)`.

6. Redefined standard function

`#define gets(I) fgets((I), 99, stdin)` makes the call look like the unsafe
`gets`, while it really reads at most 98 characters from standard input with
`fgets`. `simple.c` calls `fgets` directly.

7. A tokenizer with an optional "expected" argument

`T(s)` only moves to the next token when `s` is `0` or equals the current
token. Calls like `t(41)` and `t(93)` therefore mean "skip a `)`" or "skip a
`]`, if one is there", and quietly do nothing otherwise. This is why the
program does not complain about the unbalanced input shown in the README.
`simple.c` names this `next_token(expected)` with a `TOKEN_ANY` constant for
the unconditional case.

8. Implicit `int` and missing declarations

Old K&R C rules let `M,W,V;` declare three `int` variables with no type, and
let `D()`, `T(s)` and `main()` be functions returning `int` without saying
so. `atoi` is used without including `<stdlib.h>`. `simple.c` gives every
variable and function an explicit type, but keeps the original headers.

9. Recursive descent hidden in a single line

The declaration parser is a tiny recursive descent parser: `D()` calls itself
for `(` ... `)` groups and for each `*`. The pointer text `"ptr to "` is
printed after the recursive call returns, while the `func returning` and
`array of` suffixes are printed inside, so the English comes out in the
order C actually binds the operators. Squeezed onto one line without
whitespace, this structure is very hard to see.
