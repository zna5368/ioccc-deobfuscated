# 1994/westley

Difficulty: Easy

1. Compiler error messages as output

`#include "...You are in a debris-filled..."` never names a real file. The
compiler cannot find a file with that name, so it stops with a "No such file or
directory" error that repeats the name. The game text is that file name, and
the error message is the only output.

2. Program that does nothing when it compiles

`#if O main(){} #else ... #endif` wraps the whole game. Compiled with `-DO`, the
file is only `main(){}`, an empty legal program. Without `-DO`, the game is
compiled instead, and that never builds a program.

3. Compiler `-D` options as player input

`-Dname` defines the macro `name`, so `-Drop` defines `rop` and `-Dwarf`
defines `warf`. Every word in the game starts with "D", so the source tests
the cut-off names `rop`, `warf`, `escribe`, `onut` and so on. In `simple.c` the
macros `DROP`, `DWARF`, `DESCRIBE`, ... stand for the cut-off names.

4. Undefined macros count as zero

In `#if`, a name that is not a macro is treated as 0. A word that was not typed
is therefore false and a typed word is 1, so `#if rop&onut` checks "drop" and
"donut" without any `#ifdef`.

5. Bitwise and arithmetic operators as logic

Because every word is 0 or 1, `&` and `|` work like "and" and "or", as in
`rop&onut&((own&rain)|warf)`. `rop+epress+ime+eposit+rink+aiquiri+onut==1`
adds the words up to check that exactly one of them was typed. `simple.c` uses
`&&` and `||` instead.

6. Whitespace padding instead of newlines

A file name cannot hold a newline, so each line of text is padded with tabs and
spaces until it fills an 80 column terminal line and wraps onto the next. That
is why the original needs 80 columns with 8 character tabs. The strings look
like one long line in the source. `simple.c` keeps the padding exactly but shows
each message's screen layout in a comment above it.

7. Nested `#if`/`#else` chains

The "describe" part is seven `#if`/`#else` blocks nested inside each other with
seven `#endif`s at the end. This is a plain "first match wins" list, which
`simple.c` writes as one `#if`/`#elif` chain.

8. Behaviour that depends on the compiler

Compilers that stop at the first missing include show one message. Compilers
that keep going show every message that applies, followed by the opening
description (the last `#include` in the action part), like an adventure game
in "verbose" mode. `simple.c` keeps the separate `#if` blocks so both kinds of
compiler give the same output as before.

9. Messages mixed into the logic

Each long message is written right inside the `#if` that picks it, so the
conditions are hard to follow among the lines of whitespace. There are also
24 empty lines before the first `#if`. `simple.c` gives each message a name
(`MSG_DOOR`, `MSG_ESCAPE`, ...) with `#define` and includes it by name, for
example `#include MSG_DOOR`. That is a "computed include": the macro turns into
the quoted file name before the compiler looks for the file.
