# 2025/yang3

Difficulty: Hard

1. Shaped source code (ASCII art)

The whole program is laid out as a picture of a character, with spaces and
line breaks wherever the art needs them. The compiler ignores most of this
whitespace, so the shape tells you nothing about how the code is structured.
Statements are broken in odd places, such as `w=` on one line and `w-  4` on
the next.

2. Macros that glue tokens together

`#define _(ly,co)ly##co` joins its two arguments into one word, so
`_(type,def)` becomes `typedef`, `_(ch,ar)` becomes `char`, `_(m,ain)`
becomes `main`, `_(f,or)` becomes `for` and `_(&,&)` becomes `&&`. Keywords
are hidden because they never appear in one piece in the source.

3. Stringizing macro

`#define Q(q)#q` turns whatever is passed to it into a string literal, so
`Q(L)` is just `"L"`. Long pieces of the generated program are written as
`Q(...)` instead of quoted strings, which lets them hold spaces and run over
several lines of the picture without looking like strings.

4. Single-letter names and a `typedef` for `int`

`typedef int i;` lets `i` mean `int`, and parameters are written as `i(d)`
(extra parentheses around the name). Every variable has a one-letter name
(`X`, `x`, `w`, `a`, `l`, `n`, `u`, `t`, `p`, `q`, `r`), and the same name
`d` is reused for the argument count, a nibble difference and a string
pointer in different places.

5. Comments hiding things in plain sight

`#include/*@_@ oPxW*/<stdio.h>` puts a comment between `#include` and the
header name, and `/*44495630*/` sits in the middle of an expression. They
also break simple tools that look for `#include <...>` lines.

6. Substitution cipher in the output function

All text is printed through `o()`, which does not print characters as they
are written. Spaces and newlines are thrown away (only tabs survive), the
capital letters `T`..`Z` become `$` followed by the lower-case letter
(Ruby/Perl variable names), and `J K L M N Q R S` are replaced by computing
`(b*12 ^ 150) % 53`: `L` becomes a space, `Q` a newline, `K` a comma, `M` a
`$`, `N` a `#`, `S` a `"`, and `J` a backslash. So `Q(N i n c l u de...)`
really prints `#include...`. In simple.c the strings are stored already
decoded and printed with `fputs`.

7. Disguised numbers

Constants are written in confusing ways: `3 * 5 * + 10` is 150, `065` is the
octal number 53, `~-X` means `X-1`, `01` is 1, and characters are compared by
subtracting their codes (`*d-92` checks for a backslash, `*++d-114` for `r`).

8. Long chains of the `?:` operator

The escape-sequence decoder `e()` is one huge nested conditional expression
that subtracts offsets from the character (`a-4`, `a+4`, `a-2`, `a+16`,
`a+8+9`, `a-6`) to test for `v n t b a x`. The hex digit function `D()`
uses bit masks (`d&~39`) instead of simple range checks. In simple.c these
became a `switch` and a plain `hex_value()` function.

9. Comma operator and side effects inside conditions

Statements are packed into expressions joined with commas, such as
`X+= u & 1?o("\t"),8- ~-X % 8:(o("L "),1)`, which prints a character and
returns how far the column moved in one expression. The `for` loops also do
their real work inside the condition and increment parts, for example
`for(a=1<<l; --a+1; u=...)` and `for(;t-X%16;o(Q(L)))`.

10. Brute-force search written as nested `for` loops

Finding the right mix of tabs and spaces (so GCC and Clang see different
column numbers) is done by trying every bit pattern `a` of length `l`, where
each bit picks a tab or a space. The loops have no braces or clear names, so
it is hard to see that they are a search. simple.c splits this into
`column_after()` and `find_tab_pattern()`.

11. Pointer swapping to pad the shorter message

`p=*p?p:q; q=*q?q:p;` quietly makes the message that has ended point at the
other message, so both keep going until the longer one is finished. This
one line hides the handling of messages of different lengths.

12. Generated code that is valid in several languages (polyglot)

The printed text is at the same time C, Ruby and Perl. Comments in one
language (`/* ... */` in C, `#` in Ruby/Perl) and strings (`$t="*/ ... /*"`,
`%q[ ... ]`) hide the other languages' code. This makes the strings inside
the encoder very hard to read, since they are not meant to make sense as C.
