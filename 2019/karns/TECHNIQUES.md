# 2019/karns

Difficulty: Medium

1. Strings hidden in an integer array

The array named `_int` (which is a `char` array despite its name) holds every
string the program uses as a list of decimal numbers. For example
`27,91,37,100,59,37,100,72,0` at index 25 is `"\033[%d;%dH"`, the ANSI code that
moves the cursor. `_int+18` is the clear screen code and `_int+34` is the set
color code `"\033[%dm"`. The program passes pointers into the middle of the array
(`z1(_int+25, ...)`) so you have to count elements to see which string is used.

2. Single characters looked up from the data array

Character constants are read from the array instead of being written directly:
`_int[66]` is `'!'` and `_int[67]` is `' '` (written as `0x20`). The number 32 used
as the green color code is also `_int[67]`. Other places use octal (`040` for a
space) or plain numbers (`64` for `'@'`, `88` for `'X'`, `p0(47)` for `'/'`).

3. Library functions renamed through function pointers

At the top of `main` the standard functions are stored in pointers with
meaningless names: `z1` is `printf`, `p0` is `putchar`, `g4` is `fflush`, `mv` is
`malloc`, `y3` is `stdout` and `s0` is `stdin`. A call like `z1(_int+34, 36)`
hides that it is really `printf("\033[%dm", 36)`.

4. Spaghetti control flow with goto

There are no loops for the main logic. The program jumps between labels named
after random words (`cow`, `pic`, `china`, `tokyo`, `wow`, `chin`, `z`, `tww`,
`ten`, `pio`), and the code is written out of order: it starts with `goto cow`,
which is near the bottom. The queue loop of the search is `tokyo`, the loop over
neighbors is `wow`, and the path tracing loop is `tww`.

5. Meaningless and misleading names

Variables have names like `dd` (width), `h` (height), `hh` (queue), `qq_` and `qf`
(queue head and tail), `trundle` (parent of each cell), `p5` (the grid) and `oo`
(a scratch array holding width, height, x, y, neighbor count and the neighbors).
Unused labels `chew` and `chun` are placed in the middle of statements to look
important.

6. Shadowed variables

Inner blocks declare new variables with the same names as outer ones. Inside the
reading loop `char*s` (a line buffer) hides the integer `s` (the start cell), and
the neighbor block declares its own `dd`, `h` and `i`. You must track the braces to
know which variable a name refers to.

7. Visited flags split across two arrays by parity

Instead of one "discovered" array and one "done" array, the program uses `vp5` and
`nqd` and swaps their roles depending on whether the cell number is odd or even:
`(i&1?vp5[i]:nqd[i])` is "discovered" and `(c&1?nqd[c]:vp5[c])` is "done". Since no
cell uses both arrays for the same thing, this is the same as two plain arrays.

8. Side effects hidden inside conditions

The loop that finds `@` and `!` has an empty body. All of its work happens in the
condition, using `||1` to force a true result after an assignment:
`((d==64?s=...:d==33?e=...:d==10?p5[...]=040:13)||1)`. Likewise
`(feof(s0)&&(h--|1))` decrements the line count only when the end of the file is
reached.

9. Dead code disguised as cleanup

The last line looks like it restores the terminal colors by building
`"\033[0m\n"` in `_int` and printing it. But the chain of `&&` contains
`(_int[5]=0)`, which has the value 0, so `&&` stops there and `printf` is never
called. The strings `"%c"`, `"Goodbye :-)\n"` and several UTF-8 box drawing
characters in the array are never used at all.

10. Operator precedence traps

In the checks for the cells above and below, `y+1<h&&p5[i+dd]==_int[67]||p5[i+dd]==_int[66]`
groups as `(bounds && is space) || is goal`, so the goal test is not protected by
the bounds check. The horizontal checks use parentheses and do not have this
problem, which makes the four checks look alike while behaving differently.

11. Code formatted into shapes

The whole program is squeezed into a block with random spacing that breaks
tokens across lines (`p5   [i+dd]`, `z1(_int+25,1+i/dd,1+i%\ndd)`), and the comments
`//@` and `//!` are placed in the source so that the program can be run on its
own source code as a maze.
