# 2024/ferguson2

Difficulty: hard

1. One giant multipurpose struct

`struct a` has about twenty members (`w`, `c`, `s`, `x`, `h`, `S`, `N`, `A`,
`v[9]`, `e`, `a`, `z`, `k`, `j`, `l`, ...). Every function takes a single
`struct a *` and returns a `char *`, and each function uses different members
for different jobs. In `F()` the member `w` holds the English word, in `Y()`
`w` is a pointer that walks along a string, and `l` is argc in `Z()` but an
array index in `F()`. The same struct type is also used for the dictionary
linked list (`w` = English, `c` = Navajo, `e` = next). Because one name means
different things in different places, you can't tell what a member holds just
by reading its name.

2. Anonymous compound literals as function arguments

Arguments are never passed directly. Each call builds a temporary unnamed struct
on the spot, like `Z(&(struct a){ .v = { t, I }, .l=c, .h=a, .j=f })` or
`a->v[2](&(struct a){ .w=a->a->w, .c=a->x })`. The real arguments are hidden
among designated initializers, and any member left out is silently zero. For
example, `t()` calls `I()` without setting `.z`, so the dictionary list is never
passed along and words read from a file are always spelled letter by letter.

3. Function pointer tables

The struct holds an array `v[9]` of function pointers, so calls look like
`(*a->v)(...)`, `a->v[1](...)` or `a->v[a->e==0](...)`. The function that runs
depends on what an earlier caller put into the array: `main()` stores `w` (the
Atbash decoder) for `F()`, stores `t` and `I` for `Z()`, and `Z()` stores `Y` (the
word matcher) for `I()`. You have to follow the values through several calls
to work out which function is really being called.

4. Arithmetic on string literals

Numbers are written as maths on the character codes of string literals.
`*"1"` is 49, `*"9"` is 57 and `*""` is 0. So `z+-*"1"-*"1"- -97` is just
`z-1`, `calloc(1**"0"-47, ...)` is `calloc(1, ...)`,
`k = (s*2)**"9"-171+*"9"` is `k = 0`, and `1/ *"s"` is 0. Long index
expressions like `B[J][((((s-1)**"1")**"1")**"1")**"1"]` all come out to 0 or 1.
`'e'-':'` is the character `'+'`, `'k'-'*'` is `'A'`, and
`('\0'+'!'-1)` is a space.

5. Dead branches with a constant macro

`#define X 0` makes every `X ? a : b` choose `b`, but the `a` side is still
written as a long, believable expression. That doubles the amount of code you
have to read for each such index or call.

6. Bit tricks to set a constant

In `main()`, `c=((~~!!~!9u|2**"9"-15u/ *"0"-2u)<<~~!~!~9u),c&=9u` uses
stacks of `~` and `!` plus string-literal maths. All it does is set argc
(`c`) to 1, so that `Z()` starts reading at `argv[1]` and stops at the NULL
pointer that ends `argv`.

7. Short-circuit logic and comma operators instead of if/else

`Z()` is a single statement:
`while (a->h[a->l++]) (((*a->h[--a->l]=='e'-':') && (*a->v)(...) && ++a->l) || (a->v[1](...), ++a->l));`
The loop condition increments the index, the body takes it back with `--`,
and each branch of the `&&`/`||` adds it again. It is really just
"if the argument starts with `+`, translate that file, otherwise translate the
word". Other loops hide their bodies in conditions too, for example
`while (!isalpha(*a->w) && *a->w++);`.

8. Index swapping (`i[a]` instead of `a[i]`)

C treats `a[i]` and `i[a]` as the same thing, so the code writes
`(z++)[a->a->c]`, `z[a->x]`, `D[a->s]` and `(a->l)[a->h]`. These read like
indexing into a number, which hides which value is the array.

9. Encoded data file

The `dict` file is scrambled with an Atbash cipher on the upper case letters
(A becomes Z, B becomes Y, and so on), so opening it shows nothing readable.
`w()` undoes it when the file is loaded, using
`"ZYXWVUTSRQPONMLKJIHGFEDCBA"[c - ('k'-'*')]`.

10. Single-letter names and misleading layout

All functions (`F`, `Z`, `I`, `Y`, `t`, `w`) and globals (`z`, `J`, `C`, `k`,
`H`, `D`, `L`, `s`, `n`, `o`, `g`) have one-letter names, and some share a
letter with struct members (`c`, `w`, `s`, `z`, `k`). Global integers like `z`
and `s` get reused as loop counters in different functions. The source is
shaped into big letters (spelling `IOCCC`), with tokens such as `*` and `"1"`
split across lines, so things like `s/ *"1"` look like the start of a comment.
`putchar(('\0'+'!'-1))` is laid out to look like `putchar('\0')`.

11. Goto into shared tail code

In `I()`, once a dictionary match has been printed the code does `goto n;` and
jumps past the whole letter-by-letter spelling loop to a label near the end of
the function. This makes it look as if both paths always run.
