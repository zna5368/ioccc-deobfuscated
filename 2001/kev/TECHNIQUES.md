# 2001/kev

Difficulty: Medium

1. Look-alike identifier names

Almost every variable is a mix of `o`, `O`, `i`, `I`, `l`, `L` and `_` such as
`oO`, `Oo`, `Il`, `lI`, `iL`, `Li` and `___`. The letters look like paddles and
balls, and they are hard to tell apart from each other and from the digits `0`
and `1`. For example `o` is the ball's column, `O` is its row, `Oo` and `oO` are
its speeds, and `ii`/`Ii` are the two paddle positions.

2. Code shaped like a paddle

The source is laid out as a tall, narrow column 16 characters wide that looks
like a pong paddle turned on its side. Lines are cut in the middle of words with
backslash line continuations (`struct socka\` + `ddr_in`), which only works
because the preprocessor joins the lines back together before compiling.

3. Token pasting macros

`O0(M,W)` uses `##` to glue names together, so `O0(o,O)` becomes
`oo=(o+=Oo)-Oo`, which moves the ball and saves its old position in one line.
`l1(M,W)` builds `IL.tv_sec` or `l.tv_usec` from pieces, hiding which field of
the time structure is used.

4. Recursion to fill a string

`___()` builds the paddle text `[=====]` with a recursive function that writes a
`'\0'` on the way down and an `'='` on the way back up, instead of a simple loop.
`_[0]='['` and `_[lI-1]=']'` add the ends afterwards.

5. Reusing the paddle string as message data

The paddle string `_` is also the network protocol. `write(lL,_,1)` sends `'['`
(moved left), `_+lI` sends the `'\0'` terminator (moved right), `_+lI-1` sends
`']'` (did not move) and `_+1` sends `'='` (quit). The receiver checks
`*L==ll[_]` and `*L==1[_]`, which use the strange `index[array]` form of
`array[index]`.

6. Copy loop used as memset

The macro `L1()` is a byte-by-byte copy loop. It copies the `'\0'` at
`_[lI]` into every byte of the socket address, which is just a hidden way to
clear it to zero. The same macro also copies the host address bytes.

7. Side effects hidden inside expressions

Many assignments happen inside conditions or function arguments.
`ii>(ll=0)` resets the opponent's move to zero while checking the paddle
position, `Il-=Il` sets `Il` to zero inside a `mvprintw()` call, and
`write(lL,_,-(--Il))` sets the move to `-1` and uses its negative as the length.
`iL=!--iL` turns the argument count into a "am I the server" flag.

8. Sign flip with bitwise not

`oO=~--oO` reverses the ball's vertical direction. Decrementing and then taking
the bitwise complement of a number gives its negative (`~(x-1) == -x`).

9. Misleading `if` with an empty body

`if((Ll=select(...)));{...}` has a semicolon right after the condition, so the
`if` does nothing and the block below it always runs. The read of the
opponent's message is not really controlled by `select()`.

10. Time arithmetic packed into one expression

The time spent waiting for a key is computed in one long conditional expression
that subtracts two `timeval` structures in place, borrows a second when the
microseconds go negative, and then subtracts the frame time. The program then
sleeps for whatever is left of the frame.

11. Reused variables

Variables are used for more than one job. `II` holds the address size for
`accept()` and later the frame time, `iI` is a loop counter while copying the
host address and later the bottom row, and `Ll` holds both the key that was
pressed and the result of `select()`.

12. Infinite loop disguised as a condition

`while(_)` tests the address of the array `_`, which is never null, so it is an
endless loop that only ends with `break`.

13. Redundant operators and pointer noise

Expressions like `+0`, `+O`, `4-3`, `&*&IL`, `*(&(*L))`, `(*(&il)).sin_family`
and `(&(*LI))->h_addrtype` do nothing extra and are only there to make the code
harder to read.
