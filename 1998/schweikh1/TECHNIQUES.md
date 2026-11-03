# 1998/schweikh1

Difficulty: Hard

1. `#line` used to make string and number constants

`#line N "text"` tells the preprocessor that the next line is line `N` of a
file called `"text"`. After that, `__LINE__` is `N` and `__FILE__` is
`"text"`. The program uses this to hide its constants. `#line 4 "info\0r"`
makes `__FILE__` the data file name for `freopen`. `#line 1 "#%s%s"` makes
`0[__FILE__]` the comment character `'#'` and `__LINE__+__FILE__` the
`sscanf` format `"%s%s"`. `#line 8 "<%s>:\n"` gives both the `printf` format
and the number 8 (the length of `"#define "`). The clean version writes these
strings and numbers out directly or as named macros.

2. Hidden NUL bytes inside `__FILE__`

Some `#line` names hold a `\0` so that one string carries two or three
strings, reached with an offset. For example
`"0cc -ansi -E -dM -undef %s /usr/include/%s>r\0 ("` holds the command
(`1+__FILE__`), the temporary file name `"r"` (`__FILE__+43`) and the
delimiters `" ("` (`45+__FILE__`). gcc stops `__FILE__` at the first `\0`, so
the third piece is lost and `45+__FILE__` reads past the end of the string
into padding, which acts as an empty set of delimiters. The clean version
uses `NAME_DELIMITERS ""` with a comment so the output still matches.

3. Strange index order

`0[__FILE__]`, `2[__FILE__]` and `__LINE__[__FILE__]` use the rule that
`a[b]` is the same as `b[a]`. With `#line 1 "#\n"` they are `'#'`, the
string's ending `'\0'` and `'\n'`. The clean version writes the characters.

4. Array sizes built from `__LINE__`

`char f[__LINE__][X(*)]` with `X(x)` being `__LINE__ x __LINE__` turns into
`f[10][10 * 10]`. `L[4*X(<<)]` is `4*10<<10`, which is `40 << 10`, and on the
next line `e[X(<<)]` is `e[11<<11]`. The sizes
depend on which line of the file they are on, so moving the code changes them.
The clean version uses named size macros.

5. `case __LINE__:` labels

The case values of the `switch` on the `sscanf` result are line numbers. The
`#line` directives are placed so that the cases come out as 2 and 3 (the number
of fields read). Two `case __LINE__:` labels on following lines are different
values, so the same text can appear twice in one `switch`. The clean version
writes `case 2:` and `case 3:`.

6. Macro that finishes a statement

`#define x ) == 0 ?__LINE__:0){O =__LINE__;break;} }` closes the parentheses
of an `if`, finishes the body and closes a block. It is put after a half
written call like `strcmp (a[c], I x`, so the code does not look balanced.
The `t(a)` macro uses `x` three times and pastes `n##a` to get the counter
`nn` or `ne` for the list `n` or `e`. The clean version replaces both macros
with the function `matches_any`.

7. Digraphs and odd whitespace

`%:` is a digraph for `#` and `%:%:` is a digraph for `##`. The first lines
start with form feed and vertical tab characters, which C counts as
whitespace. The `H(x)` macro pastes `st` and `x` to make header names like
`<stdio.h>`, but those `#include` lines are commented out and the Makefile
supplies the headers. The clean version uses normal `#include` lines.

8. Odd control flow

`switch (strchr (F, S)!=0) case 1: ++ne;` is an `if` written as a `switch`
with no braces. `goto O` leaves the second loop, and the label `O` has the
same name as the local variable `int O` (labels have their own name space).
The clean version uses `if` and `break`.

9. Unused values to keep lint quiet

`int O = printf (...) + sprintf (...); O += system (K);` and
`nn = fclose (H);` store return values only so that lint does not warn that
they are ignored. `fclose` returns 0 so this also resets the counter `nn` for
the next header. The clean version keeps the `fclose` reset and stores only the
`system` result.

10. One letter names

`f`, `e`, `n`, `K`, `L`, `F`, `S`, `I`, `H` and `O` are renamed to
`standards`, `always_reserved`, `header_names`, `command`, `line`, `which`,
`standard`, `name`, `macros` and `status`.
