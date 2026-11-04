# 2011/akari

Difficulty: Terrifying

1. Source code that is also its own input image

The whole file is drawn as ASCII art, and the program's job is to keep every
other character of every other line. So the comments and strings are packed
with extra letters set exactly between the real code. When `akari` shrinks
`akari.c`, those letters are what is left, and they make `akari2.c`. Shrinking
again gives `akari3.c` and `akari4.c`. Comments like
`/*2w0,1m2,]_<n+a m+o>r>i>=>(['0n1'0)1;` and the junk part of the
`#include` line (`#uinndcelfu_dset<rsitcdti_oa.nhs>i/_*/`) look random, but
they are the hidden programs spread out over two lines. The comments also hold
a brainf\*\*\* program made of the `+ > [ ] .` characters. The rewrite
drops all of this because only the first program's behaviour has to match.

2. The program logic is a state machine stored in a string

There are no real `if` tests for "is this an odd row" or "is this the end of
a header line". The string `y` is a table of 25 states with 6 bytes per state.
In each state, one byte holds flag bits: bit 1 means "output the current
byte", bit 16 means "reset the counter to the image width", bit 2 means "count
down by one", bit 8 means "branch on newline", and bit 4 means "branch on the
counter being zero". The other bytes give the next state as a letter where
`` ` `` is 0, `a` is 1 and so on (`u = y[u] - '`'`). Odd mode reads column 0
and even mode reads column 1 (`u += n > 3`). One loop runs the whole table,
so the header skip, the row and column counting, and all three image formats
are hidden in it. The rewrite turns these states back into a header skipping
loop and two plain loops that flip a row parity and a column parity.

3. The state table is encrypted and interleaved

The first 101 table bytes are not stored as is. The loop
`y[i*2] ^= "~hktrvg~..."[i] ^ y[i*2+1] ^ 4` XORs every even byte with a key
string, with the odd byte next to it, and with 4. Then
`for(a=k=u=0;y[u];u=2+u) y[k++]=y[u];` packs the even bytes together and throws
away the odd bytes, which were only filler. The last states are stored as
plain characters with no XOR. Text such as `yuriyurarararayuruyuri` and
`daijiken` (phrases from the anime Yuru Yuri, where Akari comes from) is the filler that hides the
table.

4. Strings and fopen modes hidden inside other strings

The file open modes are never written as separate strings. `"rb"` is the last
two bytes of the table string (`y+298`). `"wb+"` comes after a hidden `\0` at
the end of the header format string `"P%d\n%d\40%d\n%d\n\00wb+"`, and the code
gets it with `d+14`. The same format string is used both for `sscanf` and for
`fprintf`. Spaces are written as the escape codes `\40` and `\x20` in the
error message. The rewrite uses normal string literals.

5. Confusing ways of writing simple tests and constants

The test `!(k-6&&k-5)` means "k is 5 or 6". `m[1][0]-'-'||m[1][1]!='\0'`
means "the name is not `-`". `(n+14>17)?8/4:8*5/4` picks state 2 or 10 based
on `argc > 3`. `")]i>(w)-;} {..."[8]==59` is always true because byte 8 of
that string is `;`. `if(1&&...)`, `1*(r)`, `+8&*(y+u)`, `01` for 1, and
`(int)'`'` for 96 are just noise. The return value
`k- -1+(-0x01)` is simply `k`, the result of `fclose`.

6. Dead code and junk assignments

`i=a=(u)*11&255;` sets two variables to a junk value that is overwritten
right away. After the `return` there are empty blocks `{;{ }}` and stray `;`.
The rewrite removes them.

7. Single letter variables reused for different jobs

`k` is the loop counter while the table is unpacked, then the netpbm magic
number (5 or 6), then the header line counter (set to 3), then the pixels left
in the row, and at the end the `fclose` result. `u` is the unpacking index, the
output width, the start state and then the position in the table. `r` is the max
value from the header and then the current byte. `i` is the image height and
then the buffer position. The rewrite gives each job its own named variable.

8. Comments everywhere to break up tokens

`int/**/main(int/**/n,char**m)` and `return+printf(...)` use empty comments
and unary plus to glue code into the picture shape without changing meaning.
Long comments are put in the middle of expressions such as
`fputc(/*...*/1*(r),q)` and `u=y[u/*...*/]-(int)'`'` so that the real code is
hard to see.
