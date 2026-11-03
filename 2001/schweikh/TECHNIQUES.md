# 2001/schweikh

Difficulty: Medium

1. Single return expression

The whole matcher `m()` is one `return` statement with no `if`, loop or local
variable. Every decision is folded into one long expression, so you have to
work out how it is grouped before you can see the separate cases (a `*` in the
pattern, the end of the string, and a normal character).

2. Nested ternary operators

`*t-42 ? *s ? ... : !*t : ...` puts one `?:` inside another in place of an
`if`/`else` chain. The inner `?:` belongs to the "pattern is not `*`" branch,
and the last `:` is the `else` of the outer one. In simple.c these are written
as separate `if` blocks.

3. Short-circuit `&&` and `||` as control flow

`m(s,t+1)||*s&&m(s+1,t)` means "try letting `*` match nothing; if that fails
and there is still string left, let `*` eat one character and try again".
`&&` only runs its right side when the left side is true and `||` only when
the left side is false, so they work as hidden `if` statements. Likewise
`... && m(s+1,t+1)` only recurses when the current character matches.

4. Subtraction instead of comparison

`*t-42` is nonzero exactly when `*t` is not 42, so it takes the place of
`*t != 42` as the ternary's condition.

5. ASCII codes for characters

`42` is the ASCII code for `'*'` and `63` is the code for `'?'`. Writing the
numbers hides which glob characters are being checked.

6. Reversed comparison and bitwise OR

`63==*t|*s==*t` puts the constant first and joins the two tests with bitwise
`|` rather than logical `||`. Because each `==` gives 0 or 1, `|` gives the
same answer here. Since `|` binds tighter than `&&`, the whole OR is the left
side of the `&&` that follows.

7. Recursion in place of loops and backtracking

There is no loop anywhere. Moving through the string and pattern is done by
calling `m()` again with `s+1` and/or `t+1`. When `*` tries "match nothing"
first and falls back to "match one more character", the call stack handles the
backtracking that would otherwise need extra bookkeeping.

8. Implicit int and no headers

`main` and `m` have no return type and `m` is used before it is declared,
which relies on the old C rule that missing types default to `int`. No headers
are included because the program only talks to the outside world through
`argv` and its exit status. Short names (`c`, `v`, `s`, `t`, `m`) give no hint
of what anything is for.

9. Exit status as the only output

The program prints nothing. `!m(v[1],v[2])` flips the match result so that a
match gives exit status 0 (shell "true") and no match gives 1. The `c<3?1:`
check returns failure when fewer than two arguments are given.
