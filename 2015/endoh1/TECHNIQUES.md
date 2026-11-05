# 2015/endoh1

Difficulty: Insane

1. Source code drawn as a picture

The whole file is shaped into a portrait, so statements are cut at whatever point
the outline needs. Holes in the picture are filled with spaces, and string literals
are split with `"` `"` pairs or `\` at the end of a line, even in the middle of a
word such as `prin\` `tf`. Comments such as `/*Turing*/` and `<--mouth` are only
decoration. Some of the padding even lands inside the JavaScript string, as in
`y=e.               pageY`. `simple.c` lays the code out normally, and keeps only the
spaces inside the string because they are sent to the browser.

2. Macros that are split across the file

The last line, `Alan Turing  (1912--1954)`, looks like a caption but is real code.
`Alan` is defined on line 1 as the start of a `for` header, and `Turing(x)` is
defined near the end as the rest of it. The argument `1912--1954` is thrown away.
Together they make the loop that reads from the browser. You have to join the two
macros together to see that the `for` at the end of the line above has a body.

3. Macro definitions hidden by empty comments

`#define/**/Alan/**/(fflush(0)...` puts a comment where the space normally goes. A
comment counts as a space, so `Alan` has no parameters even though a `(` seems to
follow it. `#define/*hah` `aha` `<--mouth*/` hides the same thing behind a long
comment, so it looks like part of the art. These lines, and the `#include`, also sit
in the middle of a declaration or a statement. The preprocessor removes them first,
so the C code on the lines above and below joins up across them.

4. Compiler options that look like something else

`-Delta_u="..."` looks like a long option, but it is `-D` followed by the name
`elta_u`. That is why the source says `(elta_u)` and never mentions `Delta_u`. The
equations of the model are not in the C file at all. The macro `laplacian(u)` is
`l##u`, which glues two tokens together to name the variable `lu`. `simple.c`
defines a default for each of these options, and `laplacian(u)` gives the name
`laplacian_u`.

5. Macros that finish an expression

`-DRGB=255:128:192` is used in `n<2?!n?RGB`, which becomes
`n<2?!n?255:128:192`. The colons of the "colour" are the colons of a nested
conditional, so this picks a number by channel. In the same way, `UV_DROP` is
`-0.5,0.5`, so `*e+=UV_DROP` is a comma expression: it adds the first number to `u`,
and the second number becomes the value that `1[e]+=` adds to `v`. `UV_BACKGROUND`
uses this trick too. `simple.c` keeps them, since they are the documented options,
but puts each one in a small function with a comment.

6. Global variables reused for unrelated things

Every variable is a global with one letter, and they change meaning. `b` is the
line read from the browser, then the Base64 answer, then the bytes of a WebSocket
frame. `u` and `v` are the drop position, then the substances in a cell, then a
colour value and the mouse position. `c` starts as `argc`, then counts two passes,
then counts bytes of output. `simple.c` gives each job its own named variable.

7. One big loop instead of functions

Almost all the program is in `main`, as `for` loops with empty bodies or bodies
that are another `for`. The real work is in the header and step of the loop, joined
by commas, such as `e+=5-j%4*2,z=TIMESTEP,*e=u+z*(elta_u),...`. The step part runs
after the body, so you have to read each line out of order. `simple.c` splits this
into functions, such as `read_request_headers`, `simulate_step` and `send_frame`.

8. Conditional expressions used as `if` statements

`i>9?p=q+i-2,p[8]=h(...):(t[i]=t[9-i])` and
`i<30?q[...]|=...,0:(n=*q++,b[i-30]=...)` do completely different jobs in each
branch. Only the side effects matter, and the `,0` is there to make the two
branches have matching types. The header scanner even uses `?:` as the loop test,
so the same expression compares text, stores bytes and decides when to stop.
`simple.c` uses `if` statements and separate loops.

9. A loop counter that selects several behaviours

The output loop runs `c` from -2 to 65543 and decides what to do from the value:
negative values produce `124-c*3`, which is 130 and 127, the first two bytes of a
WebSocket frame. Values 0 to 7 produce `c==5`, which is the 8 byte length 65536.
Larger values produce pixels, with `n=c%4` picking the colour channel. Likewise, the
inner loop `for(c=0;c+2;...)` runs twice, with `c` shifting `e` between `u` and `v`.
`simple.c` writes the frame header, the pixels and the two substances separately.

10. A two dimensional grid hidden in pointer maths

`d` is one flat array of floats. An expression such as
`e=d+i+512+i/504*8+j%4-c--` turns a single counter into a row and column, skips the
border, picks which of two copies to use, and picks a substance. Neighbours are then
reached with `e[-508]`, `*e`, `e[8]` and `e[516]`, measured from the cell to the left.
`e+=5-j%4*2` then moves to the other copy. `simple.c` uses
`grid[y][x][buffer][substance]` and loops over `x` and `y`.

11. Array indexing written backwards

`1[e]` means the same as `e[1]`, because `a[b]` is defined as `*(a+b)` and addition
works in either order. It is used to make the code harder to scan.

12. Constants disguised as arithmetic

`308438*881` is `0x10325476` and `3285377520` is `0xC3D2E1F0`, two of the SHA-1
starting values, written differently from their neighbours so that they do not stand
out. `7*2*9` is 126, and `m[97]=480` sets the size for `fgets` and stores the SHA-1
message length in bits at the same time. `simple.c` writes these values in hexadecimal
or as named constants.

13. Constants calculated by a hidden square root

SHA-1 needs four fixed numbers that do not appear anywhere in the source. The loop
`for(;n-k;n=(((1LL+(9<<i/20)/8)<<60)/k+k)/2)k=n;` is Newton's method for a square
root. `(9<<i/20)/8` gives 1, 2, 4 and 9 for each group of 20 rounds, so this finds
the square roots of 2, 3, 5 and 10 times 2 to the power 60, which are the four
constants. It runs inside the hash loop, using leftover values as its first guess.
`simple.c` writes the constants directly as `0x5A827999` and the rest.

14. Sliding window instead of swapping variables

SHA-1 normally moves five variables along on each round. Here `p` walks along the
array `t`, so `p[1]`, `*p`, `k`, `p[-2]` and `p[-3]` are those five variables, and
writing `p[2]` adds a new one. `t[i]=t[9-i]` copies the state in reversed so that
this works. The sum at the end is hidden in `t[81-i]+=(i>76)*(*p=h(*p,30*(i<80)))`,
which multiplies by 0 for most rounds. Two extra rounds exist only so that this
line can finish the sum, and this is why the second block starts at `m[82]` and
the length is at `m[97]`. `simple.c` has a normal `sha1_process_block`.

15. Packing several tables into one formula

One expression chooses the SHA-1 mixing function for all four groups of rounds:
`i/+20%2?*p^k^p[-2]:(*p&k)|((i<20?~*p:*p|k)&p[-2])`. The `/+20` is only a division
with a useless plus sign. The Base64 alphabet is replaced by
`n<62?n-37+"fl!"[n/26]:n*4-205`, where the letters in `"fl!"` are offsets for upper
case, lower case and digits, and the final part gives `+` and `/`. The bit splitting
in `t[i%5]%g<<4>>(30+i%5%3*2-i/5*6)&63` handles all 27 digits with one shift
formula. `simple.c` uses `if` statements, a plain alphabet string and a bit by bit
loop.

16. 64 bit numbers standing in for 32 bit numbers

`typedef unsigned long long l` is the only integer type, but SHA-1 works on 32 bit
numbers. Junk builds up in the top half of each value and is cleared only where it
matters, with `v%g` in `h` and `t[i%5]%g`. `l(h)(l v,l (a))` adds to the confusion
with brackets around the function name and a parameter, and `l` is both a type and
the start of names like `lu`. `simple.c` uses `unsigned int` and a function called
`rotate_left`.

17. Reading a frame by overlapping it with old data

`for(...;++c<b[6]+7;v=b[c]^(b[c+4]=getchar()&127))u=v;` reads a frame and removes the
mask at the same time. Each byte is stored 4 places after the mask byte it needs. The
loop limit reads `b[6]`, which is filled in by the loop itself after two bytes. Before
that it holds an old value that just happens to be large enough. `u=v` keeps the
second last value, so at the end `u` and `v` are the two payload bytes.
`simple.c` reads the length first, then the rest, then picks out the bytes.

18. Strings used as arrays, with casts in unexpected places

`(l)"sec-websocket-key: "[i]==(n|32)` indexes a string literal directly, and the
cast applies to the character, not the string. The `|32` turns upper case letters
into lower case. The same is done with the GUID, which has the SHA-1 padding byte
`\x80` hidden on the end, so the padding looks like part of the text. `simple.c`
gives these names and adds the padding byte in its own statement.
