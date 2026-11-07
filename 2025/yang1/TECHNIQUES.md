# 2025/yang1

Difficulty: Terrifying

1. Every line written twice (cut-proof source)

Nearly every line of `prog.c` comes as a pair, such as
`int  u,b,  e,l  ,_;typedef char ... *r/*/      //` followed by
`int u,b,  e,l  ,_;typedef char ... *d//j   R ... *///jGV.shif`. The two copies
end in different comment markers (`/*/`, `/*//*/`, `//*/`). `/*/` opens a block
comment, and the next line's `*/` (hidden inside `/*/` or `//*/`) closes it, so
normally the first copy is real code and the second copy sits inside a comment.
If one line is deleted, the comments line up differently and the other copy
becomes the real code. This is why the file still compiles when any single line
is removed.

2. Comment markers that read two ways

`/*/` can either start a comment (the `/*` part) or end one (the `*/` part),
depending on whether the compiler is already inside a comment when it reaches
it. `/*//*/` is a complete, empty comment, `//*/` is a line comment when outside
a comment but closes a comment when inside one. The program mixes these so the
same characters mean different things on different lines, which makes it very
hard to tell by eye which code is active. Running the preprocessor (`cpp -P`)
is the only reliable way to see it.

3. The file includes itself

`#include __FILE__` appears several times. `__FILE__` is the name of the file
being compiled, so the program pastes itself into itself. The guards
`#ifndef w`, `#if !defined(w)`, `#elif 41==__LINE__&&defined(EOF)` and
`#elif !defined(EOF)` decide what each pass does: the first pass includes
`<stdlib.h>` and `<stdio.h>` (which defines `EOF`), and a later pass, now that
`EOF` exists, produces the actual C code. Reading the file top to bottom
therefore does not show the order in which code is really compiled.

4. Detecting a deleted line with `__LINE__` and macros

`#elif 41==__LINE__` only matches if no line above it has been removed, and the
macro `V` is defined to `1` or `0` depending on which copy of the
`#define V` lines at the bottom survives. These become the line `_ |= V;` and
the macro `w(...)`, which either keeps its argument (`#define w(W) W`) or
swallows it into a `printf("%.0s", ...)` that prints nothing. The variable `_`
then selects the behaviour: `0` removes one line, `-1` acts like `head`, and
`1` acts like `tail`. In the unmodified program `_` is always `0`, so
`simple.c` only keeps the "remove one line" code.

5. Hidden programs in other languages (polyglot)

The text after `//j`, inside back-quotes, and after `$q=` / `$s=` is Ruby and
Perl code (for example `ARGF.reverse_each.drop(-n)` and
`$s="\xe2\x9c\x82"`), and runs of `+ - < > [ ] .` in comments are a
brainf.ck program. C ignores all of it because it is in comments, so it is
just noise when reading the C code.

6. A second C program in a dead branch

Lines 29 to 40 sit inside `#elif 0`, so the C compiler never sees them. They
are a separate program (`int I=4*2,i; void O(const char*c){...}`) that writes a
9999 line prime classifier. Its text is compressed: characters `P`..`[` index
into the table `o[]` of common words such as `"define "` and `"#include"`, and
letters `A`..`D` are turned into other characters with `(i*105^99)*3%53`.
It looks like part of the main program but has nothing to do with it.

7. Meaningless single letter names and typedefs

The globals are `u` (target line), `b` (buffer size), `e` (bytes in the
buffer), `l` (line counter), `_` (mode) and `d`, `p`, `q`, `r` (pointers).
`typedef char C;` and `typedef void P;` hide the real types, and the function
that throws away the front of the buffer is just `t()`. `simple.c` renames
these to `target`, `buffer_size`, `buffer_used`, `line`, `discard_before()`
and so on.

8. Reusing variables for unrelated jobs

`main(int z, C**v)` uses `z` first as the argument count, then as the
character read by `getchar()`, then as the end-of-file flag `z=e<b`. `v`
(the argument list) is set to `NULL` after the number is read, and the loop
`for(u=_=0;;)` runs a second time; on that pass `!v` is true, which is how the
program knows it should start processing input. `l` is used both as the line
counter and as the index `v[l=1]`.

9. Expressions instead of normal tests

`EOF-(z=getchar())` is used instead of `(z=getchar()) != EOF`;
`l += 10==z` adds one to the line count only on a newline (character 10);
`l-u` means "`l` is not equal to `u`"; `d-q--` checks that `q` is not at the
start of the buffer and moves it back in the same step; and
`q -= q[l=-1]==10` sets `l` while stepping over a final newline. Conditional
statements are written as `?:` expressions with `(P)0` (a do-nothing void
value), as in `_||l-u? (P)0 : t(r-=q-d);`.

10. Tricky return values and constants

`return 01 | fputs(...)` and `return(1)|printf(...)` print a message and return
a nonzero exit code in one expression. `01` is octal for 1, `01/+31` is a
roundabout `0`, and the string `"{line_n" "um" "b" "e" "" "r}"` is split into
pieces that the compiler glues back into `"{line_number}"`.

11. ASCII art layout

The code is drawn as a picture of a character holding scissors, so tokens are
spread over lines with large gaps, for example `{line_co" "unt}` and
`( z=getchar     ())`. Spaces are harmless to the compiler, but they break up
the code so it no longer looks like normal statements.
