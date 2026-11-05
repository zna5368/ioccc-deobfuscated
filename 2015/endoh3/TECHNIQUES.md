# 2015/endoh3

Difficulty: Hard

1. `main` is an array, not a function

`U(main)[32768]` declares `main` as an array of numbers, so there is no `main()`
function anywhere in `prog.c`. The name is still what the C startup code looks
for, and the array is reused as 64 KiB of memory for the program being run.
`simple.c` keeps the name because the linker needs it, and explains why in a
comment.

2. A constructor runs the program before `main`

`__attribute__((constructor))` is a GCC extension which asks for a function to be
called automatically before `main`. The function `x` loops forever, and only ever
leaves through `_exit`, so the startup code never gets as far as jumping to the
array. Anyone looking for the starting point in the usual place finds nothing.
`simple.c` calls this function `run_emulator`.

3. Tentative definition overridden at link time

`U(main)[32768]` has no initial value, so with `-fcommon` it is only a tentative
definition. When `mullender.c` is linked in, its `short main[] = {...}` replaces
the array, even though it is in another file, has a different type and is much
smaller. `prog.c` therefore contains no sign of the data it runs. This is also
why the linker prints a warning about the size of `main` changing.

4. Instructions stored as plain numbers

The numbers in `mullender.c` look like data, but each one is a machine
instruction. `prog.c` fetches them with `n=*C(*r)` and decides what to do by
looking at the bits. A reader must know that there is a machine being imitated
before any of the code makes sense. `simple.c` gives each pattern a name such as
`IS_MOV`.

5. Decimal numbers instead of octal

Instruction encodings of this machine are always written in octal, where each
digit is a field of three bits. `prog.c` writes them in decimal, which breaks up
the fields: `548==n>>6` is octal `01044` (TRAP), `n>>6==47` is octal `057` (TST)
and `n>>12==14` is octal `016` (SUB). `simple.c` uses octal so they can be found
in a reference manual.

6. Padding numbers with extra operators

`n%+64*-2`, `R(+6)`, `R(2*3)`, `n>>12==+14` and `001` all add symbols which do
nothing. They are there to make the code fit the shape of the source, and they
hide that the same `6` is used each time. `simple.c` removes them.

7. The whole program lives in the `for` header

`for(;;*r+=2,*r+=...,n>>6==47?...:0)n=*C(*r);` has a body which is one assignment
that reads the next instruction. Everything else, which is the actual work, is in
the third part of the `for`, the part that is normally just `i++`. Commas join the
pieces together. `simple.c` has a normal loop body with one step per line.

8. Nested conditional expressions instead of `if` statements

The instruction decoder is one long chain of `a?b:c?d:e?f:0`, and the macro `A`
is another one, `i?i<2?C(v):i-4?...:C(v-=2):&v`, where the conditions are also
inverted. `i-4?` means "if `i` is not 4" and the first `i?` means "if `i` is not
0", so the case for zero is at the very end. `simple.c` uses an `if` chain for the
instructions and a `switch` for the addressing modes.

9. Macros that modify their arguments

The macro `A(v,i)` runs `v+=2` and `v-=2` on its argument, so `v` must be a
variable and not a value, and every use of the macro changes a register. `R(x)`
passes `r[7-(n>>x&7)]` as `v`, so that long expression is pasted in seven times,
and `R` is used five times. After the preprocessor has run, a single line grows to
over a thousand characters. `simple.c` replaces the macros with the function
`operand_address`, which uses a pointer to the register.

10. Registers stored backwards

Register number `k` is kept in `r[7-k]`, so the program counter, which is register
7, sits at `r[0]`. That is what allows it to be written as a bare `*r`, which looks
like a pointer and not like a register. Likewise `r[7]` in the call to `write` is
really register 0. `simple.c` stores them in the natural order and names the two
registers used directly `PC` and `R0`.

11. Word memory accessed through byte addresses

`C(x)` is `(U*)((char*)main+(x))`. It casts to `char*` so that the addition counts
bytes, then casts back so that the read gets two bytes. The emulated machine uses
byte addresses, but the array has two-byte elements, and `main[x]` would be wrong.
Only a little-endian computer reads the two bytes in the right order. `simple.c`
names this `memory_at`.

12. Work hidden in the value of a calculation

Function calls are placed inside arithmetic so that they look like values. In
`0*write(...)+4` the result of `write` is thrown away by multiplying by zero, and
the expression is really "call `write`, then skip four bytes". In
`usleep(10)+n%64==4` a delay is added to a comparison, relying on `usleep`
returning 0. In `_exit(write(2,...))` the number of bytes written becomes the exit
status. `simple.c` makes these into separate statements.

13. Instructions that do nothing except side effects

`n>>6==47?*R(0):...` reads a value and ignores it. It looks useless, but
expanding `R` may change a register, and that is the only effect of this
instruction here. Similarly, every instruction that is not recognized falls off
the end of the conditional chain as `:0`, and so does nothing. `simple.c` has
comments in both places.

14. Reliance on unspecified order of evaluation

`*R(0)=*R(+6)` and `*R(0)-=*R(2*3)` change registers on both sides of the
assignment, and C does not say which side goes first. The result depends on the
compiler. GCC finds the source address, then the destination address, and only
then reads the source in the first case, but reads the source before finding the
destination in the second. `simple.c` writes this order out as separate statements
in `move` and `subtract`, so that the result no longer depends on the compiler.

15. Declarations with unneeded parentheses

`U(main)[32768]` and `U(x)()` are legal ways of writing `U main[32768]` and
`U x()`. The parentheses make them look like calls of a function named `U`, which
hides that they are declarations. `typedef unsigned/*int*/ short U` also gives a
one letter name to the type, with a comment suggesting the wrong one.

16. Preprocessor directives in the middle of a statement

`__attribute__((` is at the end of one line and `constructor))` is seven lines
later. All the `#define` lines sit in between, in the middle of the declaration.
They are removed before the compiler sees the code, so the two halves join up, but
a reader sees macros where the declaration should continue.

17. A comment that continues a macro across lines

A `#define` normally ends with its line. In `#define C(x) (U*) ((/* ... -dpd`, a
comment begins inside the macro and ends on the next line. Comments are removed
before directives are read, so the macro carries on with `*/char*) main +(x))`
with no backslash to show it. A similar comment sits between `#include` and
`<unistd.h>` and even contains backslashes that do nothing, and a simple search
for `#include <` misses it.

18. Hints written in comments

`/*| |IO| -dpd` and `|CC| ll*/` hold the text `ll-dpd`, which is `pdp-11` when
turned upside down, in the middle of what looks like decoration. Other comments
such as `/*lian*/` and the many `/**/` only take up space.

19. Split string literals

`"Illeg" "al ins" "truction ;-" "(\n"` is one string. C joins neighboring string
literals, so it can be broken at any point. This stops a search for the message
and lets it follow the shape of the source. `simple.c` has the whole message in one
piece.

20. Source code shaped as text

The code is laid out as large letters reading `IOCCC` above `1984`. The layout has
no effect on the program, but it forces gaps into the middle of expressions, such
as `A(r[  7-(n       >>x&  7)]`, so they are hard to read as one piece.
`simple.c` is formatted normally.
