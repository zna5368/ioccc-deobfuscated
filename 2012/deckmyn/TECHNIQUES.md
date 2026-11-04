# 2012/deckmyn

Difficulty: Insane

1. No variables except `argc` and `argv`

The program declares no variables at all. Every counter is stored inside the
command line strings. `**C` (the first byte of the program name) is the row
counter, `(*C)[1]` is the clef loop counter, `(*C)[2]` holds the note duration
and `(*C)[4]` remembers whether the bass clef was chosen. The first bytes of the
font string `C[1][2..5]` hold the number of staves, the number of tokens read
in the current row, the staff line byte and the note position. The integer
`argc` (`c`) is reused as a byte counter and later as the distance between the
current row and the note. In `simple.c` all of these are normal local
variables with names.

2. Reusing `argv[3]` as a pointer

`(C[c]=C[1]+2)` uses `argc`, which is 3, as an index. `argv[3]` is the NULL
that ends the argument list, and it is overwritten with a pointer into the
font string. This pointer is then moved forward and back (`*C[3]++`,
`C[3]-=2`) so that `*C[3]` means a different variable at different times. The
expression `C[3][1,-1]` uses the comma operator, so it is really `C[3][-1]`,
the staff counter one byte before the pointer.

3. Integers hidden in a string with a macro

`#define C(c) ((int*)(C[1]+6))[c]` casts part of the font string to an `int`
array, so `C(0)` to `C(3)` act like four `int` variables for the widest staff,
the current column, the start column of a token and the byte inside a token.
The macro `C` and the array `C` share a name with the variable `c` and the
macro `c`, which makes every line look the same.

4. `printf` as a macro and for its return value

`#define c(C) printf("%c",C)` prints one byte. `printf` returns the number of
characters written, so `c(52*c('C'+'4'/4))` prints `P`, gets 1 back, and
then prints `52` (`4`), giving the `P4` header of a PBM file. The 1 that comes
back is also stored as the starting staff count. Inside the drawing loop
`C(1)+=c(...)` moves the column forward by one each time a byte is printed.

5. Font hidden in the whitespace of the source

The source code itself is passed as the first argument. From character 99,
each group of 8 characters becomes one byte, where a space is a 1 bit and any
other character is a 0 bit. This is done in place over the font string by the
first `for` loops. Because the first byte is cleared before its first bit is
read, that bit is always 0. In `simple.c` this is `decode_font`, which writes
into a separate `font` array.

6. Magic numbers for glyph locations

The glyphs are found with offsets like `C[1][7***C+151+C(3)]` (quarter rest),
`'c'*5+2*'%'` (569, the sharp sign) or `C[1][42+*(*C+1)+3***C]` (bass clef).
Most glyphs share a sheet that is 7 bytes wide, so `7*row` picks a line of the
sheet and the small offset picks the glyph. `simple.c` gives each glyph a named
offset such as `TREBLE_CLEF` or `FLAG_UP_STEM`. The treble clef is stored with
its bits flipped and is drawn with `~`.

7. Characters and numbers mixed randomly

Numbers are written as characters and characters as numbers: `75` is `'K'`,
`70` is `'F'`, `58` is `':'`, `46` is `'.'`, `' '` is the stem bit 32, and
`5*51` is 255. The note position formula `105-p-7*(p<'c')-18*(p<77)-7*(p<'C')`
turns a note letter into a staff step, with `C..B` one octave and `c..b` the
octave above.

8. Mixed ways of indexing arrays

The same byte is written in many forms, such as `*C[2]`, `C[2][0]`,
`(*(C+2))[0]`, `**C`, `C[0][0]` and `(&*C[0])[0]`. All of them mean the same
thing, but they make it hard to see that two parts of the code use the same
value.

9. One giant nested conditional

Everything drawn after the clef is a single `printf` argument made of about
800 characters of nested `?:` operators. It chooses between bar lines,
accidentals, rests, notes (head, dot, stem, ledger lines, flag) and time
signatures, and combines the parts with `|`. In `simple.c` it is split into
`draw_token` and one small function for each kind of symbol.

10. Changing the input while reading it

The first pass over the music changes the music string. Each `x` (new staff)
token gets its first character set to 0, and the amount of an `s` (space)
token is turned from a digit into a number with `C[2][1]-=48`. The second pass
depends on these changes: a 0 token means "pad to the end of the staff", and
the space amount is read directly as a count of bytes.

11. Rewinding a pointer for every row

The image is printed one pixel row at a time from top to bottom, so every one
of the 80 rows of a staff must read the same tokens again. The program counts
the tokens read in a row (`C[1][3]++`) and at the start of the next row moves
the music pointer back with `C[2]-=3**C[3]`. The `3**C[3]` looks like a power
but is `3 * *C[3]`.

12. Shaped source code

The code is formatted into a picture. Because the spaces are the font bits, the layout cannot be changed without breaking the
font, and lines are split in the middle of names, numbers and even a string
(using a backslash at the end of the line).
