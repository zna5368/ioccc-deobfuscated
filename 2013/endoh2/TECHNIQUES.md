# 2013/endoh2

Difficulty: Hard

1. Second program hidden in a comment

Almost all of `endoh2.c` is one comment, from `/*@@@sssss8]]` down to
`return@0+0;}*/`. The compiler ignores comments, so the real code is only the
last few lines. The comment is still data: the Makefile runs
`./endoh2 < endoh2.c`, so the program reads its own source file and pulls the
comment back out. This also dodges the contest size limit, because the size tool
does not count that comment. `simple.c` names this step `read_payload`.

2. Magic byte offsets into its own source

`for(;e<3583;e++>15&&e<3582&&d>32?...` only keeps bytes 16 to 3580 of the input.
Those numbers are the exact position of the comment contents in the file, so
adding a single character to `endoh2.c` breaks the program, and nothing in the
code says what the numbers mean. `simple.c` calls them `SOURCE_BYTES`,
`PAYLOAD_START` and `PAYLOAD_END`.

3. Stand-in characters

A comment cannot safely contain `*/`, so every `/` in the data is written as `_`
and turned back with `d-95?d:47` (95 is `_`, 47 is `/`). All whitespace is thrown
away on input by `d>32`, which is what lets the comment be drawn in any shape, so
a real space in the template is written as `@` and restored with
`(e=*p++)-64?e:32`. `simple.c` uses `SLASH_STAND_IN` and `SPACE_STAND_IN`.

4. Custom compression stored as printable text

The first 823 bytes of the data, such as `@@@sssss8]][[[[a#`, look like noise but
are a small compressed format, similar to LZ77. After `d=*p-88`, each byte is one
of four instructions: below zero means "these five bits are the data", 0 to 5
means "write a run of ones", 6 to 35 means "copy from a little way back", and
above 35 means "copy from the row above". The length and the distance of a copy
are both squeezed into one number, read back with `d/9*6` and `d%9*-12-12`.
`expand_layout` in `simple.c` has one labelled branch for each instruction.

5. Several cases folded into one expression

The four instructions above share a single loop. The length comes from
`d<0?5:d<6?d>4?160:30:d>35?27<<(d-36):d/9*6`, a chain of `?:` operators used in
place of `if` statements. The value comes from
`*q=d<0?*p>>e&1:d<6||q[...]`, where `||` does double duty: `d<6` being true gives
1 for the run of ones without looking at the right side, and otherwise `||` turns
the copied value into 0 or 1. `simple.c` writes these as separate `if` branches
with their own loops.

6. Work done in the wrong part of a loop

In `for(;e<3583;e++>15&&e<3582&&d>32?*h++=d-95?d:47:0)d=getchar();` the loop body
only reads a character. The test, the counting and the storing are all packed
into the increment section of the `for`, using `&&` and `?:` as an `if`. The
trailing `:0` exists only because `?:` needs a second branch. Readers expect that
section to just count, so it is easy to miss that it does the real work.

7. One expression that is the whole output stage

The last loop is a single `putchar(...)` with five levels of `?:`. It chooses
between a newline, a space, a template character, a quote and a character of the
embedded string, and updates pointers along the way. `print_program` in
`simple.c` spells out those same choices as an `if`/`else if` chain.

8. A pointer used as a hidden state machine

`r` is both a position and a memory of what has happened so far. It starts as
null, meaning the string has not been opened, `r&&*r` means it is in the middle
of the string, and `(r=r?p++:w,34)` moves it to the next state. The comma operator
runs that assignment and then gives the value 34, the quote character, to print.
This is how the program prints its own data as a string literal inside the
program it generates. `simple.c` uses `string_text` and a commented branch.

9. Code that depends on where the last loop stopped

Nothing sets `p` before the output loop. It works because the previous loop
happens to leave `p` at `w+823`, the first byte of the template text. In the same
way `q=h+108` skips a blank first row with no explanation. `simple.c` starts from
`buffer + COMPRESSED_BYTES` and `layout + ROW_CELLS` by name.

10. Reused single letter variables and one shared buffer

`e` is a file position in the first loop, a bit counter in the second and a
temporary character in the third. `d` is a character and then an instruction. `w`
holds the text and, after `h`, the expanded bitmap as well, so the same array has
two meanings. `simple.c` gives each job its own local variable.

11. Numbers in place of characters

ASCII values are written as plain numbers: 10 for newline, 32 for space, 34 for a
quote, 36 for `$`, 47 for `/`, 64 for `@` and 95 for `_`. The check
`g%109%108<2` is a compact way of asking "is this the first or last column?",
since `%109` gives the column and `%108` folds column 108 onto 0. `simple.c` uses
character constants and `column == 1 || column == ROW_CELLS`.

12. Data decoded but never used

All 823 bytes are expanded even though only the first 56 rows of the result are
read here. The rest is font data for the generated program, so a reader trying to
explain every decoded bit will not find a use for it in this file. The loop still
has to run to the end, because that is what leaves `p` at the template text.

13. Source drawn as a picture, with comments as filler

The text is arranged into a shape, so line breaks fall in the middle of tokens and
expressions, as in `*h\n++=d-95`. Filler comments such as `/*IOCCC*/` and
`/*IOCCC2013*/` pad lines out to the outline and even split an expression between
`:r` and `&&*r`. `simple.c` uses ordinary formatting.
