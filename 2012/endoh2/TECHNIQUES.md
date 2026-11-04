# 2012/endoh2

Difficulty: Hard

1. ASCII art source layout

The whole program is drawn in the shape of a spigot. To make that work, the
long string `s` is cut into many small pieces like `"i,j,k,n"     ";char*q"`.
C joins string literals that are next to each other, so the spaces between the
pieces are not part of the string. The pieces only exist to fill in the picture.

2. One string holding several strings

`s` contains `\0` characters, so it is really four strings stored back to back:
the font table plus the start of the child program, the pi code, the e code,
and the end of the program. `main` picks them out with the magic offsets
`s+168`, `s+281` and `s+386`, and `sprintf(a,"%s%s%s",...)` glues the chosen
ones together (each `%s` stops at the next `\0`). The `@` after each `\0` is
padding so that `"\0@3"` is not read as the octal escape `\03`; the offsets
point just past the `@`.

3. Quine through printf

The generated program has to contain its own source. The program text is used
as a `printf` format string and is also passed to itself as the `%s` argument:
`sprintf(b,a+22,a,34,32,24)`. The `%s` puts the raw text (still holding `%%`
and `%s`) inside the string literal, while the format copy turns `%%` into `%`
for the real code. `%c` with `34` and `32` write a `"` and a space without
having to escape them, and `%d` writes the digit count for the next generation.
`a+22` skips the 22 character font table at the start of the text.

4. Encoded shape strings

The strings `f` (pi) and `g` (e) describe the picture of the generated program.
Each character is a run length: `(c-34)%77` characters long, made of spaces if
`c > 111` and of program text otherwise. For example `z` is 11 spaces and `L`
is 42 characters of code. After most runs a `"` is printed so the code inside
the string literal stays valid. `n` (76 characters of code) and `~` (15 spaces)
are long runs that get no quote, so two characters can make one longer run.
After the first 62 (pi) or 45 (e) characters the picture is past the string
literal, so `r-t<(t-g?62:45)` stops adding quotes. In `simple.c` these are
decoded into arrays of numbers where negative means spaces.

5. Padding the include lines with comments

The `#include` lines must stay on their own lines but are shorter than the
65 character picture width. `sprintf(a,"%.33s/*%.28s*/...",b,b+66,...)` copies
33 characters of each include line and fills the rest with a comment made from
28 characters of the declarations that follow. The `%.Ns` format prints at most
N characters, so slices of the text can be taken without copying them first.

6. Compressed font table

The ten digit glyphs (3x5 pixels each, 15 bits) are stored in just 20 bytes:
`G1%xJ{;Q7wunmuGuu%uu`. The bitmap for digit `n` is
`275*q[n+10] - 8*q[n] - 8`. The author searched for printable characters that
make this formula produce every glyph (see `find-font-table.rb`). It is only
used by the generated program, but it rides along at the front of the string.

7. Comma and conditional expressions instead of statements

Code such as `(q-b)%66<1?*q++=10:0;` and
`*r-110&&*r-126&&r-t<(...)?*q++=34,(...):0;` replaces `if` statements with
`?:` and `&&`, and joins several actions with commas. Comparisons are written
as subtractions (`*r-110` means `*r != 'n'`) and `%66<1` means "is zero".

8. Reused variables

`m` is first the argument count, then the inner loop counter. `a` and `b` swap
roles: `a` holds the plain text, then `b` holds the expanded program, then `a`
gets the padded text and finally `b` is overwritten with the drawn output.
`p` and `q` walk through these buffers as input and output pointers.

9. Hidden line breaks

The output buffer gets a newline before every 66th position, including
position 0. That is why the result is printed with `puts(b+1)`: the first
character is a newline that has to be skipped.

10. Generated code made to fit two shapes

The child program's code is written so it still compiles when the text is cut
at any line ending in either picture. It has extra `;;` and `;;;;`, unused
variables like `e` and `y`, and odd spellings like `j=+puts(d)` so that the
pieces line up in both the pi and e shapes.
