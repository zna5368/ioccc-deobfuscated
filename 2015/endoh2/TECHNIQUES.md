# 2015/endoh2

Difficulty: Medium

1. Backspace characters that hide code from the terminal

`prog.c` contains hundreds of real backspace bytes (ASCII 8) inside its strings.
A backspace moves the terminal cursor one column to the left, so the next
character is drawn on top of the old one. In `s[999]="i^H^H...nt main(){ printf(~~)^H^H..."`
each piece of code is followed by the same number of backspaces as it has
characters, which puts the cursor back at column 19. The next piece then covers
it. Because of this, `cat prog.c` shows only the last text written to each
column, and a short line appears where 829 bytes of code really are. An editor
shows the backspaces, and `wc -c` shows the true size. `simple.c` names each run
of backspaces, such as `BACKSPACES_21`, and puts one code piece on each line.

2. Overstrike emphasis

`_^HH^HH_^He^He...` writes an underscore, a letter and the same letter again in
one column. This is an old printer trick for making bold, underlined text, and
`less` still understands it, so the words are emphasized there and plain with
`cat`. Each letter takes five bytes, which makes the text impossible to find
with a search. `simple.c` writes this as `EMPHASIZE("H")`.

3. Program text stored in a string that prints itself

The string `s` is a copy of the program with `%s` in the place where `s` itself
belongs. `printf(s+62**"...",s)` uses the decoded string as the format and the
original string as the argument. The string is printed inside itself, so the
code never needs to contain a copy of a copy. `simple.c` keeps this idea in
`printf(format, template)` with separate names for the two roles.

4. Placeholder characters

A real `"` or `\` inside `s` would end the string or start an escape, so `~`
stands for `"` and `#` stands for `\`. The expression `c-35?...:92` changes `#`
(35) to `\` (92), and `*"}..."<c?34:c` changes anything above `}` to `"` (34).
The test for `~` is written as "larger than `}`", which hides what it looks for.
`simple.c` compares against `QUOTE_PLACEHOLDER` and `BACKSLASH_PLACEHOLDER`.

5. Numbers hidden as the first character of a string

`*"..."` reads the first byte of a string, so a string can be used as a number.
Almost all of each string is backspaces that exist only for technique 1. `*"^H..."`
is 8, `*"s^H..."` is 115 and `*"}^H..."` is 125. `62**"^H..."` looks like a
double pointer but is 62 times 8, which is 496. `2+*"^H..."` is 10, the newline
character. `simple.c` uses named constants and character literals instead.

6. Decoy code

The file is built to look like a greeting program, but the parts that seem to
do that do nothing. `printf("")` prints nothing. The text in
`return(*"\0...Hello...")` is never printed, because the `*` reads only the first
byte, which is the `\0` escape for zero, so this is `return 0`. `simple.c`
drops the empty `printf` and writes `return 0`.

7. Two strings in one array

`s[999]` holds the encoded text at the start and the decoded text at offset 496.
Nothing says where the second string is except `p=62**"..."+q`. It has no explicit
end either, since it relies on the unused part of the array being filled with
zeros. `p[*"..."]=0` looks like it ends the string, but it writes eight bytes
ahead into a place that is already zero. `simple.c` uses a separate `format`
array and writes the ending zero directly.

8. Whole loop squeezed into a `for` header

The decoding logic is all in the third part of the `for`, as
`c=q-s>*"..."||c>8?*p++=c-35?*"..."<c?34:c:92:0,p[*"..."]=0`. It has nested `?:`
operators, an assignment inside a condition and a comma operator, while the loop
body is just `c=*q++`. The value given to `c` is thrown away right after, since
the body replaces it. `simple.c` uses an ordinary loop with `if` statements.

9. Unexplained limits

`q<s+435` stops 21 bytes before the end of `s`, which leaves the last backspaces
out of the decoded copy. `q-s>115` removes backspaces only near the start of `s`
and keeps them after that. `c>8` is the test for "not a backspace". Both limits
are needed for the two copies to differ in the right places, but nothing in the
code says so. `simple.c` names them `DECODED_LENGTH` and `FIRST_KEPT_BACKSPACE`.
