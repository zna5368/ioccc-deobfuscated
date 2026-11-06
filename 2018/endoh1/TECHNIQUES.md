# 2018/endoh1

Difficulty: Hard

1. Font compressed by repeated pair substitution

The 956 byte string at the start of `F` is not the font itself. The first
loop of `main` runs 63 times. Each pass takes the first three characters of
the current string as a rule: wherever the first character appears in the
rest of the string, it is replaced by the next two. The result is written
right after the old string and becomes the input of the next pass, so the
rule for the next pass is hidden inside the text that the current pass
creates. After 63 passes the string has grown into the 5760 byte font (96
characters of 60 cells each). simple.c writes the font out as a table of
pictures, where `'.'` is an empty cell, `'#'` a black outline and `'o'` the
white inside of a closed letter.

2. Pixel values hidden in the remainder of a character

The font cells are ordinary printable characters, and only the remainder
after dividing by 3 matters: `M(G[...],3)` turns each one into 0
(background), 1 (black) or 2 (white). Because of this, many different
characters mean the same pixel, which is what makes the substitution
compression work well and makes the string look like random text.

3. Filler characters that are thrown away

The decoder skips every character below `'\''` (39): `*G>=13*3?...:1`. The
string uses `' '`, `'#'`, `'$'` and `'&'` to draw a picture inside the
literal, and none of them end up in the font. Only the replacement pairs are
copied without this check.

4. One buffer used for everything

`F[1<<21]` holds the compressed font, every intermediate pass of the
decoder, the final font, the input text, the image pixels and the
compressed GIF data, one after another. `G` and `I` are moved through it and
change meaning several times: `G` is first the decoder input, then the font,
then the output position of the bit writer; `I` is first the decoder output,
then the text, then the pixel buffer. simple.c uses separate arrays named
`font`, `text`, `canvas` and `dictionary`.

5. Numbers written as calculations

Magic numbers are hidden behind arithmetic: `'*'*'1'*7` is 14406, which is
written as the two bytes `"F8"` of `"GIF89a"`, `32*3+1` is `'a'`, `122*2` is
0xF4 (the screen descriptor flags), `63777` is 0xF921 (the start of a
graphic control extension), `k<<2` is 1028 (two bytes of 4), `1<<11` is
0x0800 (no local color table and LZW code size 8), `53+6` is `';'` (the GIF
trailer), `+060<<+020` is octal 48 shifted by 16, and `7-3` and `17+3` are
10 (newline) and 20 (line height). simple.c uses `printf("GIF89a")`, named
constants and hexadecimal values.

6. Encoded string literal

`"SJYXHFUJ735"` is printed with 5 subtracted from every character, which
gives `"NETSCAPE2.0"`, the name of the extension that makes the animation
loop.

7. Function names and decoy names

The functions are called `C` (write a byte), `V` (write a 16 bit number),
`E` (write an LZW code), `J` (draw a character) and `M` (remainder). The
parameter of `M` is called `nop`, and variables like `pq`, `O`, `op` and
`tOo` are set or declared but never really used. `z` is always 0 and is used
in place of the number 0. `X` holds 255 and `Y` holds 256.

8. A macro that renames the for loop

`#define L for` replaces every `for` keyword with `L`, and the define itself
has a long comment between `#define` and the name, so it does not look like
a definition at first sight.

9. Indexing written backwards

`c[t[c]<z?...:t]` uses the character variable `c` as the array and a pointer
as the index. In C, `a[b]` is the same as `*(a+b)`, so this is really
`t[c]` or `T[c]`, depending on the condition. `*"<nopqabdeg"` is the first
character of a string, `'<'`, which is 60, the number of cells in one glyph.

10. Conditional operators and commas instead of if statements

Almost every decision is a `?:` expression with several assignments joined
by commas, for example `i-7-3?I-=i<32||127<=i,j+=12:(H+=17+3,W=W<j?j:W,j=12)`
handles a normal character, an unprintable character and a newline in one
line. The LZW step `Z=c[t[c]<z?E(Z),k<(1<<12)-2?t[c]=++k,T:T:t]` checks the
table, writes a code, adds a new table entry and picks the next table row
all at once. simple.c uses `if` statements.

11. Work hidden in for loop headers

The pixel position of a glyph cell is computed inside the increment part of
the for loop in `J`, and the assignment `I[...]=k` happens there too, while
the body only reads the font. The sub-block output loop
`L(G++;k=G-r>X?X:G-r,C(k),k;)` computes the block size, writes it and tests
it in the condition. simple.c uses plain loops with the work in the body.

12. Packed index arithmetic

One counter `j` from 0 to 239 walks over all pixels of a glyph. The cell is
`j/2/2`, its column is `M(j/2/2,6)`, its row is `j/12/2`, and the pixel
inside the 2 x 2 block is `M(j,2)` and `M(j/2,2)`. simple.c uses four nested
loops for rows, columns and the two pixel offsets.

13. Global variables reused as temporary values

`Z` is the current LZW code, but `E` also uses it as its bit counter and
leaves it at 0. `i` is the input character, the palette counter and the
pixel counter. `k` is the last LZW code and then the sub-block size. `j`
is the line width while reading and the frame number while writing.

14. Code formatted into a picture

The source is laid out in a shape and filled with comments such as
`/*!"'()*+,-./12357:...*/` and `/*---#$%&04689@ABDOPQRabdegopq---*/`, which
list the characters without and with closed areas. Line breaks are put in
the middle of expressions, for example `L(j=12,r` / `=I;...` and `*r-` /
`7-3?`.

15. End of input marker left in the image

The character -1 that ends the input is left in the first byte of the pixel
buffer, so the first pixel is turned into the color -62 and looks up a table
entry before the start of `T`. That memory happens to be 0, so the first
pixel is always encoded as black. simple.c reproduces this in
`pixel_color` with a comment.
