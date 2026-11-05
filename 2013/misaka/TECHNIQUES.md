# 2013/misaka

Difficulty: Insane

1. Source code that still compiles when copies are pasted side by side

`misaka.c` is built to survive `./horizontal_cat misaka.c misaka.c > misaka2.c`,
which puts a second copy to the right of every line. Some lines end in `//`, so
the right copy is commented out, as on the `#include` lines and the line ending
`char**l){//`. Other lines have no `//`, so their code really appears twice in a
row, and the two copies nest inside each other. `for(_=p=1;p<I;p++){` becomes two
`for` loops, one inside the other, and the closing braces further down are
arranged so that the result is still valid and acts the same. To read the program
you have to work out which statements exist in which build, so `simple.c`
replaces the pasting with two numbers, `COPIES_WIDE` and `COPIES_HIGH`.

2. Mode switch hidden in a comment that only exists after pasting

The line `O &=72   /` is the widest line in the file and it ends in `/`. The
line after it begins `6/*][*/;`, so one copy of the program reads `O&=72/6`, which
is `O&=12`. `O` was set to 5 by `*&O=5`, and `5&12` is 4, which is true and selects
horizontal mode. When a second copy is pasted on the right there is no padding
after this widest line, so the `/` touches the `*` of the next copy's `*&O=5` and
forms `/*`. That starts a comment which ends at the `*/` on the next line and
swallows the `6`. The code is now `O&=72`, and `5&72` is 0, which selects vertical
mode. This is also why the statement was written `*&O=5` rather than `O=5`.

3. File that includes itself and counts its copies with `__LINE__`

The declaration `int p,q,d,b,_=0` stops halfway and the next line is
`#include __FILE__`, so the file includes itself in the middle of a statement.
On the first pass `e` is not defined, so `#ifndef e` is true and the main code is
compiled. During the include `e` is defined as `0x1`, so that branch is skipped
and only the `#elif e` branch at the very bottom is used. It adds
`|(1<<(__LINE__/45))|01U` to the initial value of `_`. The file is 45 lines long,
so `__LINE__/45` tells which stacked copy this is. Each copy sets one more bit,
so 1 copy gives `_=1`, 2 give 3, and 3 give 7. `if(1<(O=_))` then turns on long
cat mode, and the value of `O` sets the length of the cat.

4. One macro name with three meanings

`e` is first `0x1`, which marks the include pass. Then `#undef e` and
`#define e(c)` make it a macro that takes code. Near the end it is changed again
to `#define e 0`, so the `#elif e` branch of the main pass and every later stacked
copy are turned off. A reader has to know which definition is active at each line,
and the change in `#undef/*pqdz'.*/  e//` is easy to miss among the comments.

5. Code that only runs once per source line

`e(c)` expands to `if((_!=__LINE__?(_=__LINE__):0)){c;}`. It stores the current
line number in `_` and only runs `c` if `_` held something else. When the source
is pasted side by side, two copies of `e(fclose(t))` land on one line and share
the same `__LINE__`, so only the first can run unless `_` changed in between. The
same variable `_` holds other values between these checks, which makes the macro
very hard to follow.

6. Run length encoded picture

The cat is not visible anywhere in the source. It is stored as text like
`aaFbfeccdea` in the array `o`. A character above 96 such as `a` starts a group of
different characters and gives its length as `S()-96`. A smaller character such as
`F` means that the next character repeats `S()-32` times. The letters `a` to `e`
then choose a character to print, so most of the picture takes one or two bytes
per run. `simple.c` holds the picture as plain strings.

7. Lookup table hidden in an error message

The first string in `o` is `"\n\40\"8oCan\40not\40open %s\n\0aaFb...`. Its first
five characters are the only characters used to draw the cat, read by
`o[0][S()-97]`. The error message starts after them, which is why it is printed
with `printf(05+*o,l[p ])`, meaning the string `o[0]` plus 5. The picture data
starts after the `\0` that ends the message, which is why the reader starts at
`q=22`. One string literal holds three unrelated things.

8. Array size used to measure the source

`b=((sizeof(o)/sizeof(char*))-1)/4` counts how many strings are in `o`. Pasting
copies side by side adds more strings to the first four rows, but the fifth row
is followed by `//`, so it stays at one string. The result is the number of copies
across, and it is used in two ways. `p+=b` steps down to the next row of the first
copy, and `I=_-10?b:1` prints every character `b` times to make the cat fatter.

9. Change detection used to move forward

`z(~i)` moves to the next byte only if its argument differs from the saved value
`Z`. Because `i` counts down, `~i` is different on each call, so in a group of
different characters it moves every time. The same function with `Z=1` also serves
as the flag `if(Z-1)` that says a new run count must be read. The reader has to
notice that `~i` can never be 1 here to see what the code does.

10. Recycled and mirrored variable names

The names `p`, `q`, `d` and `b` are mirror images of each other, and each is used
both as a global variable and as a field in the struct `f`, so `P[p].q[q]` uses
`q` twice with two meanings. Global `b` is the read size, then a line width, then
the number of copies. `q` is an argument number and then an offset in a file. `I`
is `argc` and later a repeat count. `O` and `o`, as well as `P` and `p`, differ
only by case. Names give no hint of the job they do.

11. Numbers and statements written in odd ways

Simple values are hidden behind other spellings. Newline is `012` or `10`, space
is `040` or `\40`, and 5 is `05`. `return+1`, `return 01` and `return 0U` are all
plain returns. `*&O` is just `O`. `d=sizeof(f*)` stores a value that is never
used, and `{;}` blocks do nothing. Each one makes the reader stop and check it.

12. A second program in another language

The file is also a Brainfuck program, and `perl bf.pl misaka9.c` prints how the
copies are stacked. Brainfuck ignores everything except eight characters, and it
skips a `[...]` loop when the current cell is 0. The opening `/*[*/` starts such a
loop which ends at the `]` after `#include<stdlib.h>//`, so the C code is skipped.
Comments like `/*]*/`, `/*[[*/` and `/*][*/` keep the `[` and `]` balanced, and the
real Brainfuck instructions sit in comments such as `//]++++[->++[->+>++++<<]<]`.
They look like noise to a C reader, and none of them affect the C program.

13. Checksum of the file written inside the file

The comment `[misaka.c,size=3808,crc=d0ec3b36]` is correct: the CRC-32 of the file
really is `d0ec3b36`, even though that value is part of the bytes being measured.
A reader may look for code that checks it, but no code ever reads it.

14. Layout as a picture

The code is shaped like a figure, so spaces and line breaks are placed by the
drawing and not by the logic. Single names are broken apart, as in
`P [p].q[       P[       p]                  .o       ]=  012;`, and empty `/**/`
comments are used in place of spaces. Statement boundaries do not line up with
lines, so the shape of the code gives no clue about its structure.
