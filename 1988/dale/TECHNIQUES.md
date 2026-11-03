# 1988/dale

Difficulty: Medium

1. Macros that disguise keywords

`#define while switch`, `#define case_3 default`, `#define L if`,
`#define I goto`, `#define J else` and `#define f write` rename C keywords and
functions. The line `while(V){` looks like a loop but is really
`switch(V){`, and `case_3:` is really `default:`. The original file
(`dale.orig.c`) even used `#define _ define` so that it could write `#_`
instead of `#define`, which only worked with old preprocessors.

2. Token pasting to hide function names

`#define a(x)get##x##id())` glues its argument between `get` and `id`, so
`a(u)` becomes `getuid())` and `a(g)` becomes `getgid())`. The macro also
supplies the closing parenthesis of the surrounding call.

3. Encoded string

`q()` subtracts 7 from every character after the first one, in place.
`"/ipu6ljov"` is therefore decoded to `"/bin/echo"`, which is the program run
with `execv()` when the user is root.

4. Numbers built from expressions

Small constants are written as arithmetic on `c` (which is `1`), `A` (which
is `0`) and `l` (which is `1`). `(c*c-c+c/c)<<(c*c)` is `2`, `c*c+c` is `2`,
`A+c*c-c` is `0`, `'-'-'-'-'+'+'+'` is `0` and `'/'/'/'` is `1`. The buffer size
`l<<(1<<l<<1)<<1<<(l<<1)<<(l<<l<<l)<<1` works out to `4096`. Some of these
also include terms that cancel out, like `A-A+c-r-c+r` (always `0`) and
`A*(p)C` (always `0`).

5. System calls used only for their return values

`p Z=chroot("/")` and `Z-=kill(l)` look dangerous, but for a normal user both
calls fail and return `-1`, so `Z` ends up as `-1 - -1 = 0`. `Z` is the flag
that says "do not print a newline". Likewise `main(chroot("/tmp")+l,C)` passes
`-1 + 1 = 0` as the new argument count. The `return(A*a(g);` line returns
`0 * getgid()`, which is just `0`.

6. File descriptor shuffling

`close(0)` and `close(2)` free up standard input and standard error,
`dup(1)` copies standard output into the lowest free number, which is `0`, and
`close(1)` frees `1`. `pipe(z)` then receives descriptors `1` (read end) and
`2` (write end). As a result, every `write(0, ...)` in the program actually
goes to the screen, the parent "writes to standard error" to fill the pipe,
and the child "reads from standard output" to empty it. The pipe descriptors
are even stored into the `char` array `z`, which is later reused as the read
buffer.

7. Jumping into the middle of a switch and an if

The `switch` uses fallthrough instead of `break`, and its `case` labels are
placed inside the `if (fork())` and `else` blocks. `default:` handles `-n`
and falls into `case 1:`, which sets up the pipe and forks. The parent loops
with `goto case_2` (through `#define h case_2` and `I h`), a label sitting
inside the `if` block. The child's `else` branch falls through to `case 0:`,
which is in neither branch.

8. Recursion instead of a loop

After the child prints one argument it calls `main()` again with an argument
count of `0`, which jumps straight to `case 0:` to print the next argument.
The global variables `c` and `A` carry state between calls. The child stops
when `read()` returns `0` (an empty argument) or when it crashes running
`strlen()` on the `NULL` at the end of `argv`. Because the crash happens after
all the output is written and the parent ignores the child's exit status,
nobody sees it.

9. Comment tricks

`O/*/*/0)` contains `/*/*/`, which is just a comment containing a single `/`, so it is
`close(0)`. The original file (`dale.orig.c`) also had `for/*/(;;);/*/k()`,
which looks like a `for(;;);` loop, but `/(;;);/` is inside a comment. Old
pre-ANSI compilers deleted comments without leaving a space, which glued
`for` and `k()` into `fork()`. Modern compilers turn a comment into a space,
so `dale.c` spells it `fork()`.

10. Old style function definitions and implicit int

`q(Q)P*Q;{...}` and `main(V,C)P**C;{...}` use the pre-ANSI style where
parameter types are declared after the parentheses, and no return types are
given, so they default to `int`.
