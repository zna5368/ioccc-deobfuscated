# 2020/endoh3

Difficulty: Hard

1. Quine built with a stringizing macro

`#define/**/Q(x,y)char*q=y#x","#y")",*p,s[x;}` is the trick behind the
self-printing program. Everything after `Q(` up to `puts(s)` is the macro
argument `x`, and the string literals on the last lines are `y`. The `#`
operator turns an argument into a string, so `#x` is the program's own code
as text and `#y` is the string literals written out with their quotes and
backslashes. `q` becomes `y` (the first two lines of the file) + the code +
`","` + the quoted literals + `")"`, which is the whole source again. The same
macro also closes the array `s[9<<9]` and the `main` function with `x;}`.
simple.c builds the same text from a `header_parts` table and a `body`
string, and adds the quotes with an `append_quoted` function.

2. Clock picture drawn in the comments

The clock face and hands in the source (`12`, `L`, `o------->`, the box
drawing) are only a big `/* ... */` comment, so the compiler ignores them. The
real clock is drawn by the code. Also `#include/*...*/<stdio.h>` has a comment
in the middle, so a search for `#include <` does not find it. simple.c uses
ordinary comments and a normal `#include <stdio.h>`.

3. Self-laying-out text with an oval comment

The first loop `for(p=s;+i<1839;*q>32?...:*q++)` pours the text `q` into the
80 column screen `s`, dropping every space (`*q>32`). `750>r*r+k*k*4` tests
whether a cell is inside an oval. The value `y=(inside)*4+y/2` keeps a short
history of the last few cells, so it is 4 on the first cell inside, 6 on the
second, 7 deep inside, then 3 and 1 on the two cells after the oval. Indexing
`"0X+0X+!"[y-1]-1` (every character is one too big) gives `/`, `*`, space,
`*`, `/`, which writes `/*`, blank space and `*/`. So the oval becomes a
comment and the printed program still compiles. `r<41?...:10` puts the
newline in column 79. simple.c names the parts and stores the edge characters
as `"/W*/W* "`.

4. Sine and cosine from a hand-written Taylor series

`P()` has no `sin` or `cos`. The loop
`for(k=0;++k<39;*o*=i/6875.5/(k%2?k:-k))y=o[1+k%2]+=*o;` keeps one term
`*o` and multiplies it by `angle/k` each time, switching the sign every two
steps. Odd terms are added to `o[2]` (cosine) and even terms to `o[1]`
(sine), so it computes `22.5-r*cos(a)` and `39.5-r*sin(a)` at the same time.
`6875.5` is 43200/(2*pi), so an angle in seconds where 12 hours is one full
turn becomes radians. simple.c names this function `plot` and keeps the
series (with `float` math) so the rounding stays the same.

5. Mirrored clock

The series starts from `*o=r<0?r:-r` (minus the radius), so the sine part is
subtracted and the hands turn counter clockwise. The hour numbers come from
`"OISEA2dC8e"[x%10]`, which are the digits 0 to 9 as they look in a mirror,
and `*p+=x/10*41` turns the space after the digit into `I` (32+41) for 10, 11
and 12, so they read `OI`, `II` and `SI`. simple.c names the table
`mirror_digits` and writes the 41 as `'I' - ' '`.

6. Half-row dots with odd/even character tests

The vertical position is counted in half rows (`p=s+y+k/2*80`), so one
character holds two dots. `*p=k%2?*p%2?+59:44:*p>39?59:39` chooses `'` (39)
for a top dot, `,` (44) for a bottom dot and `;` (59) for both. It works out
the old character with tricks: `*p%2` is true for `'` and `;` (odd codes) and
`*p>39` is true for `,` and `;`. simple.c puts this in `draw_dot` and writes
the characters instead of their numbers.

7. One loop for both hands and reused globals

`for(i+=(k*60+x)*60;18+r;...,i=!r--?i%3600*12:i)P();` draws both hands in a
single loop. `r` counts down from 10 to -17; while it is 10 to 0 it draws the
hour hand at the hour angle, and when `r` passes 0 `i` is changed to the
minute angle `i%3600*12`. Because `P()` uses `-|r|`, the negative values give
lengths 1 to 17. The single letter globals `y`, `x`, `i`, `k` and `r` are
reused for everything: `k` is first the hours from `sscanf` and later the half
row returned by `P()`, and `i` is the screen position, the hour number angle
and the time. simple.c uses two separate loops and local variables with
names.

8. Odd syntax and small number tricks

`int(y)`, `float(o)[03]`, `void(P)()` and `int(main)()` wrap names in
brackets, which C allows but nobody writes. `03` is an octal 3, `9<<9` is
4608, `+i` and `+*q` have useless plus signs, and `;;` adds empty
statements. The `sscanf` format is split into `"%d"":%d:%d"`. The trailing
`,"#define..."` after `puts(s)` is just a string that is computed and thrown
away. simple.c uses plain declarations and named constants.
