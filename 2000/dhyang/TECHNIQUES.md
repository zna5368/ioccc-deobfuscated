# 2000/dhyang

Difficulty: Hard

1. Source code shaped as a picture

The whole program is laid out as an image of Saitou Hajime. Statements are
split at odd places and spread over many lines with large runs of spaces and
tabs, so the line structure tells you nothing about the code structure. Running
the file through the preprocessor and a formatter undoes this.

2. Empty `#define X` and `/**/` comments as filler

`#define/**/X` defines `X` as nothing. The letter `X` is then sprinkled through
the code (`r  <X X X X  X 768`, `d[X(143+`, `main X ()`) only to fill holes in
the picture; the preprocessor deletes every one. The `/**/` comments in
`#define/**/X` and `int/**/main` work the same way: they act like a space but
let the words be glued together visually.

3. Self reproducing program (quine) with a hidden copy of its code

The program prints a new C program that contains the same string `d`. To do
that, the code of the new program is stored inside the string, with every
character XORed with 6 (`x^(p?6:0)` in `n()`) so it looks like garbage, e.g.
`[=ohr&o*` decodes to `];int i,`. The string is read twice: first the encoded
bytes are copied as-is into the new string literal, then a `'\0'` in the
string is turned into the closing `"` (`(p=34)`), and the same bytes are read
again and decoded to become the new program's code.

4. Run-length encoded picture layout

The start of `d` is a list of one-character commands that describe the shape of
the next picture: `!` (33) means newline, a character from 35 to 92 means
"fill this many cells with program text" and a character of 93 or more means
"leave this many cells blank". The text cells are wrapped in `"` while the
program is still inside the string literal. The first character of each layout
(`X` here) is also a number: `*d+143` is where that layout ends.

5. Rotating data to cycle through several pictures

The layout commands of four pictures are stored one after another in the first
768 bytes. The new program's string starts at `(143 + r + *d) % 768`, so the
layout that was second becomes first, and each generation draws the next
picture. This is how one program turns into a chain of programs that ends in a
loop of three.

6. Decorative spaces inside the data

The string `d` itself is drawn as part of the picture, so it contains many
spaces that are not real data. The function `s()` skips them; the test
`(x^(p?6:0))==32` undoes the XOR first so that it always compares the raw byte
with a space. Because the spaces are counted in the sizes 768 and 947, the
magic numbers in the code do not match the amount of real data.

7. Nested conditional expression instead of if statements

`n()` is a single `return` with three nested `?:` operators, side effects
(`r++`, `x=...`, `p=34`) hidden inside them, and the extra `';'` (59) returned
at the end so that leftover picture cells become empty statements.

8. Single letter global names and magic numbers

All variables (`d`, `o`, `i`, `r`, `w`, `f`, `b`, `x`, `p`) are one-letter
globals, and characters are written as numbers: 33 is `!`, 34 is `"`, 32 is a
space, 59 is `;` and 10 is a newline.

9. Implicit int and missing headers

`n()`, `s()` and the variables declared after the string rely on the old C rule
that a missing type means `int`, and `sprintf` and `puts` are used without
including `<stdio.h>`. This keeps the code short enough to fit inside the
picture.
