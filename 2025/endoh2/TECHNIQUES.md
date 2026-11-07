# 2025/endoh2

Difficulty: hard

1. Source shaped like a picture

The code is laid out as a lightning bolt. Line breaks fall in the middle of
expressions and names (`z\n=1`, `x=W\n[p=j++/\n2+s]`), so you cannot read one
statement per line. Running it through a formatter is the first step.

2. Comment tricks

`/**/#include<unistd.h>/***/` puts empty comments around the includes to fit
the shape. At the end, `return /*/*\n/*/ 0;` looks like it opens two comments,
but `/*/*` starts one comment and the `*/` inside `/*/` ends it, so it is just
`return 0;`.

3. One array holds everything

`s[1<<20]` is used as three arrays. `s[0..T)` holds the value of each lattice
node, `s[T..2T)` holds each node's parent (written as `s[y+T]=x` and read as
`q[T]`), and `s[2T..)` is the list of visited nodes followed by the frontier. In
`simple.c` these are the separate arrays `cell`, `parent` and `order`.

4. Two values packed into one integer

Each node's value keeps the direction it was reached from in its lowest 3 bits
(`x%8`) and its "charge" in the upper bits (`*q/8*8`). Setting the low bits to 7
(`*q|=7`) hides a node without losing its charge. `simple.c` names this with
the `DIRECTION()` macro and an `enum` of directions.

5. A sliding pointer as two lists

`p` points into the third part of `s`. Everything before `p` is already in the
tree, and `p[0]..p[z-1]` is the frontier. The code swaps the randomly picked
node to `p[0]` and does `p++`, which moves it from the frontier to the visited
list in one step. Later `*--p` walks the visited list backwards.

6. Bit tricks for small arithmetic

`M=W-~W` is `2*W+1`, because `~W` is `-W-1`. `-~W` is `W+1` and `~-i` is
`i-1`. `T*02` is an octal `2`. In the character test, `*p%8+~i` means
`*p%8-i-1`, which is zero only when the direction equals `i+1`.

7. Neighbour offsets computed from the direction number

Instead of a table, `y=x+~-i/2*W-W-i%2+i/4` turns direction `i` (1 to 6) into
the offsets `-W-1, -W, -1, +1, +W, +W+1`. `simple.c` uses a `switch` with one
line per direction.

8. Work done inside `for` headers

The tree-growing loop picks the random node, swaps it and advances `p` in the
first part of the inner `for`, and checks the lattice border in the condition
together with `--i`. The charge loop does all its work in the step part
(`*q|=...,s[q[T]]+=...`) with only `q=s+*--p` as the body. The drawing loop
calls `printf` from the step part, and its body only sets `p` and `x`.

9. `?:` and `&&` instead of `if`

`!s[y]?s[p[z++]=y]=i,s[y+T]=x:0` is an `if` without an `else`. The brightness
(`z>N*4?1:z>N?0:2`), the underline (`x%8-3&&y%8-4?33:4`) and the printed
character (`j%M?...?32:i?47:92:10`) are nested conditionals, and they become
`if`/`else` chains in `simple.c`.

10. Characters written as numbers

The output characters are written as ASCII codes: `32` is a space, `47` is
`/`, `92` is `\` and `10` is a newline.

11. Hidden escape codes

The format string `"\x1b[%d;33;%dm%c\x1b[0m"` is split across lines. The first
`%d` picks bold (1), normal (0) or dim (2). The second is either 4 (underline)
or 33, which only repeats the yellow colour and so does nothing. Underlines
stand in for horizontal edges.

12. Values that depend on post-increment

In the drawing loop the body runs `j++`, and the step part then uses the new
`j` (`i=j&1`, `j%M`). So `i` is 1 for the left character of a node and the
newline comes at the last position of each line. `simple.c` loops on `k` and
writes `k%2==0` and `(k+1)%LINE_LEN==0` directly.

13. Variables reused for different jobs

`i` is first the start position, then the random pick, then the direction
counter, and last the left/right half flag. `x`, `y` and `z` are reused the same
way (`z` is the frontier size and later the total brightness). Each job has its
own named variable in `simple.c`.

14. Size from compiler flags

`w` and `h` are not declared anywhere. The Makefile passes them as
`-Dw=$COLUMNS -Dh=$LINES`, so they look like variables but are really numbers
fixed when the program is compiled.
