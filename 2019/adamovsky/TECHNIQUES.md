# 2019/adamovsky

Difficulty: Terrifying

1. Macros that hide keywords and common statements

At the top of `prog.c` there are short macros like `e` for `return`, `w` for
`union A`, `q` for `c=getc(p)`, `k(d,v)` for `D(d);return v;` and `j(n,r,i)`
for `A*n=R(r[i].a);`. `d(n)` and `h(n)` declare whole functions that take two
or one argument. Since the code is written using these letters instead of the
real C words, you have to expand every macro in your head before you can read
a single line.

2. Macro that hides an entire `if` block with a hidden `return`

The `s(x,y,i,q,n,o)` macro expands to a nested `if` that checks whether the
stack is being unwound, may free a node and then returns, or builds a "resume"
node and returns that. Its `if(q)k(y,x)` part looks like it only returns when
`q` is true, but `k` expands to two statements, so only the free is conditional
and the `return` always happens. Six different functions share this macro, so
the most important control flow of the program is invisible.

3. Short and deliberately misleading names

Functions are named with one or two letters (`X`, `R`, `D`, `C`, `U`, `I`, `Y`).
The `f`/`r` prefixes are shuffled between related functions, so `rA1` is really
the first step and `fA1` the resume step. The names of the call/cc and exit
builtins are swapped: `fE` is call/cc (used for the `c` character) and `fC` is
exit (used for `e`). The apply functions are named `Y` so they look like the Y
combinator. In `simple.c` all of these got names like `apply_operator`,
`callcc_builtin` and `exit_builtin`.

4. One union used for everything

Each Unlambda value is an array of five `union A` cells. The same union is used
as a one-argument function pointer, a two-argument function pointer, a pointer
to another node, and a `long long` that can be a character, a file position or
a reference count. Which meaning a cell has depends on which slot it is and
which function is stored in slot 0, so the reader must track this by hand.
`simple.c` names the slots with an enum (`FUNC`, `ARG1`, `ARG2`, `LINK`,
`REFS`).

5. Function pointers used as type tags

Instead of having a type field, the code compares the function pointer in slot
0 against function names, for example `u(f)==rT` or `u(f)!=fW&&u(f)!=fCh` in
the free function `D`. A function's address tells the program what kind of node
it is, so functions are used as data as well as code.

6. Character lookup table instead of a parser

The table `T[256][2]` maps every source character to a constructor function
and a template, using designated initializers with raw ASCII codes like
`m(115,C,fS0)` (115 is `s`) and `o(32,1)` (32 is space). The macros `m` and `o`
hide the initializer syntax. Characters that should be skipped get small
numbers (1 to 7) where only 4 means "comment until end of line". `simple.c` uses
character literals such as `['s']` and named skip classes.

7. Self-modifying parser

The constructor stored for the backquote character, `T[96][0].t`, is
overwritten at run time. It is set to `Yt` while reading the argument of `d`
(build a tree instead of evaluating), to `YE` after `e` is called (skip the
rest of the program), and back to `Y` afterwards. The same character in the
source therefore means three different things depending on hidden global
state.

8. Continuations by unwinding and rebuilding the C stack

When `c` (call/cc) is used, the code returns a special node with its fourth
slot pointing to itself. Each function on the C stack sees this, wraps its own
local variables into a "resume" node and returns it, so the whole stack is
turned into a linked list of nodes. `main` then calls that list to rebuild the
stack. Calling a continuation later rewinds the source file with `fseek` and
replays the saved list. This is why the `s` combinator and application are
split into many small functions, which makes the control flow very hard to
follow.

9. Trigraph inside a comment

`for(/**??/` followed by a line holding only `/` uses the trigraph `??/`,
which means backslash. The backslash joins the next line, so the two lines
become the empty comment `/**/`. Without `-trigraphs` the compiler sees an
unfinished comment instead.

10. Hidden second program and source layout tricks

The code after `/*/;` at the end of the file is inside a comment, but the 2018
size tool reads it differently and turns it into a second program (`prog2`)
that prints an Unlambda program. The `#define` lines also end with `//`
comments, which do nothing for the compiler but let the lines be indented and
shaped freely and confuse the size tool.

11. Packed source formatting

Statements, declarations and function bodies are packed together with no line
breaks and odd spacing so the text forms a picture. Many functions are written
on a single line next to other functions, hiding where one ends and the next
begins.
