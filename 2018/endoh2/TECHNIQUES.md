# 2018/endoh2

Difficulty: Terrifying

1. Program that reads its own source code

The whole program is passed as an argument to the macro
`#define Q(q) ... char*Q=#q; int(main)(){q;}`. The macro uses the argument
twice: `#q` turns it into a string stored in `Q`, and `{q;}` turns it into
the body of `main`. So the code runs and can also read its own text as data.
This is how the program prints itself (a quine) without holding a second copy
of its code.

2. Layout, spaces and colors are thrown away before use

The first loop, `for(;*Q>34?(*S++=*Q++):*(Q+=*Q-27?1:Q[2]/49+4););`, copies
only the characters above 34 into memory. This drops spaces, `!` and `"`, so
the parrot shape and the quote marks can be anything. When it finds an escape
character (code 27) it skips the whole ANSI color code: `Q[2]/49` is 1 for
`ESC[3Xm`/`ESC[4Xm` (5 characters) and 0 for `ESC[0m` (4 characters). That is
why the colored output can be compiled and run again.

3. A virtual machine hidden in one expression

The second `for` loop is a small stack machine written as one long chain of
`?:` operators. `M[0]`..`M[8]` are registers, `S` is the stack pointer and `C`
points to the next instruction. Each character `c` is one instruction, decided
by `t = c - 93`:

- below `]`: push a number (it uses the next character too)
- `]`..`e`: push register `t`, `f`..`n`: push register and add one to it
- `o`..`w`: pop into a register
- `x`: load, `y`: store, `z`: jump if not zero, `{`: subtract, `|`:
  remainder, `}`: less than, `~`: print a character

The real program is the 490 characters `Ler[Tsaxiyj...` that this machine runs.
simple.c is that program translated back into normal C functions.

4. Numbers made of two characters

A push instruction computes its value as `*C++%92*57-t-500` from the opcode
and the character after it. For example `0d` pushes 1 and `1d` pushes 0. No number appears in the bytecode in readable form.

5. Bytecode stored in a string that is never used

The bytecode sits in a string literal after `return+0**`. `0**"..."` is zero
times the first character of the string, so `main` just returns 0 and the
string seems useless. It is only there so that `#q` puts it into `Q`, where
the virtual machine finds it at `C=M+3800`.

6. Tables hidden in a comment

Inside the string is the comment
`/*0+)IFMNFU*+0-02;>@=,(#1555546231rst%wvu#08OCCC*/`. It is data for the
bytecode: `0+)IFMNFU*+0` is `/*(HELMET)*/` with every character moved up by
one, `-02;>@=,(#` are the start points of the ten parrots, `1555546231` are
their ANSI colors, `rst%wvu#` are the eight drawing directions and `08OC` are
the jump sizes. Each value is turned into a number with remainder math such
as `(c+44)%161-80`, so the characters look random.

7. Counter that changes the program's own code

The only thing that differs from one generation to the next is one digit in
the bytecode (the `1` in `]q1d`). It is a push instruction for the frame
number. When the program copies itself it replaces that digit with
`(c-31)%10+40`, which counts down from `1` to `(` and then starts again. The
drawing code runs the frame number plus one times, and only the last run
stays on the screen, so frame 3 draws the fourth parrot and the fourth quote.
simple.c reads the digit as `FRAME_DIGIT` and picks the parrot directly.

8. Byte pair compression

The ten parrot outlines are stored in `6Zba4ORRZ...`. It is decoded 57 times,
once for each character from `#` to `[`. Each round takes the first two
characters as a pair and replaces every copy of the current character with
that pair. After all rounds the 382 characters become 1164 drawing commands.
simple.c holds the decoded commands in `parrot_outlines`.

9. Turtle drawing and flood fill

Only the outline of each parrot is stored. Each command marks the current
cell, turns clockwise by ``c-'`'`` eighths of a circle and moves one cell, like
a turtle with a pen. `^` and `_` move 4 cells without drawing and `]` ends a
parrot. The body is filled at runtime by a flood fill that uses a stack in
memory, and the fill never spreads down into the last row. The canvas is then
printed with color codes wherever the cell type changes.

10. Shifted text for the quotes

The ten "Dead Parrot" quotes are stored with each letter one lower than the
real letter (`~` for space, ``cd`c`` for `dead`). `@` stands for `I'm`. The first
character of each quote gives its column and `/` ends it. The `!` is added by
the code because the source filter in technique 2 would remove it.

11. One memory array used for everything

`M[99999]` holds the registers, the stack, the new source text (from `M[30]`),
the canvas (from `M[1976]`), the program's own text (from `M[3557]`), the
decompressed outlines and the fill stack. The canvas overlaps the start of the
program text, so drawing erases code that has already finished running. The
program stops by storing 0 just after its last instruction, because the loop
runs only while `*C` is not zero.

12. Jumps that are always taken and shared code

There is no plain "goto" instruction. Jumps that must always happen push a
value that is never zero, such as a register holding an address. Some
sequences are shared: the end of one path pushes a value, jumps into the
middle of another path and lets that code finish the store.

13. Misleading code

The source contains `#include/*#exclude*/<stdio.h>`, `#define/**/Q`,
`int(main)()` and comments like `/*IOC CCC*/` and `/*ABCDEFGH*/` that look
odd but change nothing. The `HELMET` comment printed into the next version is
not read by anything. The variable `P` is declared but never used.
