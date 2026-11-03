# 1990/pjr

Difficulty: Easy

1. Macros that generate repetitive declarations

Three tiny macros produce almost all of the code. `A(a)` expands to the
prototype `G a();`, `B(a)` to the struct member `G (*a)();` (a pointer to a
function returning `G`), and `C(a,b)` to a whole function body
`G a() { printf(b); return X; }`. Writing `A(a)A(b)...A(N)` declares 28
functions in one line, so the real shape of the program is hidden until
the preprocessor is run.

2. Chained calls through a struct of function pointers

`struct F` (renamed `G` by a `typedef`) holds a pointer to every printing
function, and every printing function returns the global struct `X` that
was filled with those pointers. Because the return value is a struct, the
caller can immediately pick a member and call it again with `.`, as in
`g().s().v()`. Each call prints one character, so a long chain prints a
whole sentence. This looks like method chaining from other languages but is
plain C: `g().s` reads the `s` member of the returned struct and `()` calls
it.

3. Atbash letter substitution

Each function prints the letter mirrored across the alphabet from its name:
`C(a,"z")`, `C(b,"y")`, ..., `C(z,"a")`. So the call chain in `main()`
spells the message in this mirrored alphabet: `g().s().v()` prints `t`,
`h`, `e`.
Reading `main()` therefore shows `gsv jfrxp yildm ...` instead of
`the quick brown ...`. `S()` and `N()` are the only functions that print
what you would expect: a space and a newline.

4. Function names that shadow struct member names

The same one-letter names are used for the functions and for the struct
members (`a` is both a function and `X.a`). C keeps struct members in a
separate name space, so this is legal, but it makes `g().s()` look like two
global function calls when the second one is actually a struct member.

5. Pointless assignment in `main()`

`main()` assigns the final result of the chain back to `X`
(`X=g()...N();`). This assignment does nothing useful because `X` already
holds the same pointers; it only makes the line look more mysterious.

6. Unused decoy code

The author mentions a piece of code that is never used: the function
`void Q(){}` and its matching struct member `void(*Q)();`. `Q` is
declared, stored in `X`, and never called. It is just a distraction.

7. Single-letter names and dense layout

All identifiers are one letter (`A`, `B`, `C`, `F`, `G`, `X`, `Q`, `S`,
`N`), and the code is packed into a rectangle with line breaks placed in
the middle of macro calls like `A(` / `o)`, which makes the source hard to
scan.
