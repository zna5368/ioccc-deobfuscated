# 2012/hamano

Difficulty: Hard

1. Source code drawn as a picture

The code is laid out as a large dancing man, with the `#define` lines on its
head and the statements bent along its arms and legs. To fit the shape, string
literals are broken into pieces and `\` at the end of a line splices the next
line into a macro or a string. That splice also keeps the leading spaces of the
next line, so strings like `"]>>>>>>/Contents 1          0 R>>"` and the `C`
macro contain runs of spaces that are only there because of the shape.

2. Macros that hide printing and PDF syntax

`p(s, ...)` expands to `_+=printf(s, ...)`, so every print also adds the number
of bytes printed to `_`. `q(s)` turns its argument into a string with `#s` and
prints it, which lets C code such as `q(#include<stdio.h>\n)` be written
without quotes. `D(s)` wraps text in the PDF dictionary brackets `<<` and `>>`.
`S` and `Y` are the PDF keywords `endobj` and `endstream`, `C` is the whole
page object, `DANCE` is the trailer, and `o(s)` saves the current offset into
`b[s]` and starts PDF object number `s`. `E` is a `for` loop over the
characters `A` to `z`. The reader has to expand all of these in their head to
see what is written.

3. One file that is both a PDF and a C program

The output starts with `/*%PDF-1.3%*/`. A C compiler sees a comment, a PDF
reader sees the `%PDF-1.3` header and then a comment that starts at `%`. The
C part (`#include`, two `#define`s and `typedef int ET;`) sits in front of the
first PDF object, where PDF readers ignore it. After that the two formats take
turns hiding each other: `/*` hides PDF objects from C, `%` starts a PDF
comment that hides `*/` from the PDF reader, and the page text is put inside a
C string `static char*_="..."`. The PDF keywords that follow the string,
`ET endstream endobj`, are turned into C by the macros: `ET` is a type and
`endstream` and `endobj` become `main(){...return 0;}`. That `main` walks the
string and prints each character found between `(` and `)`, so running the
compiled PDF prints the original text again. The program is a generator of
this two-language file, so most of its strings are pieces of it.

4. Byte offsets kept as a side effect

A PDF ends with a table that gives the byte position of every object. The
program never stores a position on purpose. Instead `_` grows with every
`printf` through the `p` macro, and the very first print is `p(...)-2` so the
count starts at `%PDF`, two bytes in, where PDF readers count from. The
length of the page text is found with `*b=~(p(...))` before it and
`*b-=~_` after it: `~x` is `-x-1`, so the two `-1`s cancel and `b[0]` becomes
the end minus the start. `b[0]` is then printed as the first entry of the
table, where a real PDF would have a free entry, and `simple.c` has to copy
this quirk to give the same output.

5. Font hidden as tiny strings with shifted characters

The dancing man font is the array `w` of short strings like `"+*-(,&-&"`.
Each one is a line through a few points on a 10 by 10 grid. Adding 10 to each
character turns `&'()*+,-./` into the digits `0123456789`, so `"+*-(,&-&"`
means the points (5,4) (7,2) (6,0) (7,0). The function `nyan` copies a string
character by character, adds 10, puts a space after each one, and after every
second digit writes ` m ` for the first point or ` l ` for the others. Those
are the PDF commands "move to" and "line to". The last string
`"+*+.'`'@'mi +/+/(mi"` uses the same trick on letters: `` ` `` `@` `m` `i`
become `j` `J` `w` `s`, the commands for round joins, round caps, line width
and stroke, which draw the body and the head. `simple.c` keeps the points as
plain digit strings such as `"54726070"`.

6. Poses packed into bits

The array `v` has one number per letter. Its bits pick one of four shapes for
each arm and leg (two bits each) and bit 8 means the man stands on his head.
`nyan` takes the pose with `g>>n&3` and uses `|n*2` to jump to the right group
of four strings in `w`, so `n` is 6, 4, 2 and 0 for the four limbs. When bit 8
is set, `*_=105-*_` turns the digit `d` into `9-d` and flips the drawing. For
upper case letters `~y&' '` is true (bit 5 of the character is clear) and
the string `17[w]` adds a small flag, `8 7 2 2 re`.

7. Recursion and a conditional chain instead of a loop

`nyan` calls itself to draw the next limb. The return statement
`n?n-9?nyan(_,y,n-2):_:nyan(...,y,9)` counts `n` down by two from 6 to 0,
then at 0 adds the flag and calls itself once more with `n=9`. With `n=9`,
`g>>9&3|18` is always 18, which picks the body string, and `n-9` being zero
stops the recursion. The C operators `?:` and the comma are nested so the
whole thing is one expression.

8. Numbers stored in other places

Many constants come from the data arrays instead of being written out. The
font names `T` and `U` are `*v` (84) and `7[v]` (85), the `*` that closes a C
comment in the output is `*v/2` (42), and the number of letters, 26, is `25[v]`. Other
numbers are disguised: `~-c` is `c-1`, `c+~58` is `c-59`, `' '` is 32, `'@'`
is 64, `';'`/`59` is the count of widths plus one and `105`, `108`, `109` are
the characters `i`, `l`, `m`. Writing `7[v]` instead of `v[7]` works because
`a[i]` is the same as `*(a+i)`.

9. Hard to read letter test

`~-c%' '<25[v]&&!~-(~-c/'@')` checks whether `c` is a letter. `(c-1)/64` must
be 1, so `c` is between 65 and 128, and `(c-1)%32` must be below 26, which
leaves `A`-`Z` and `a`-`z`. Letters use the dancing man font `T` and
everything else uses Courier `U`. In the same way `~-c/6+~14` is zero only for
the six characters between `Z` and `a`, which get the glyph name `/A`.

10. Misleading names and odd declarations

`N`, `Nyan` and `A` are `typedef`s for `int`, `char*` and `char`, and the
function that draws a glyph is called `nyan`. The global `_` is the output
counter in `main` but a parameter named `_` inside `nyan` is a write pointer
into a buffer. `main` is declared as `N main(N c)`, so its argument count is
reused as the loop character. The `for` loops have their work in the header
and an empty body `;`, and the main read loop puts the whole `if/else` in the
increment part.
