# 2024/mills

Difficulty: terrifying

1. A virtual machine hidden in string literals

The C code is only a small interpreter. The real program is the array of
strings `E[]`, where every character is an instruction for that interpreter.
`main()` reads a string one character at a time (`while(c=*r)`) and runs each
one. To find out what the program does you first have to work out the
interpreter's instruction set, then disassemble about 1,600 instructions.

2. Instructions encoded as character arithmetic

`c-=32>c?6:79` turns each character into an opcode number. The letters `O`
to `}` become instructions 0 to 46 (`T` pushes a number, `b` adds, `z`
starts a switch and so on). The characters from space to `C` give negative
numbers, which mean "call string `E[c+47]`", so a space calls `E[0]` and `C`
calls `E[35]`. The first six instructions take the next character as an
argument with 72 taken away (`q=*++r-72`), so `TX` pushes 16 and `Tgg`
pushes 31 then runs `g` (bitwise and).

3. Recursive `main` used as the subroutine call

`main(c,E)` calls `main` again with a negative `argc` and the string table
as `argv`. The new call picks its routine with `c[47+g]` (the same as
`g[c+47]`). The first, real call is detected by `!v`: it saves the command
line arguments in `v` and starts by running the string `"C"`.

4. A macro that builds the instruction decoder

`#define L s; } {...} if(!c--) { t` closes one instruction's code and opens
the next one. Each instruction is tested with `if(!c--)` so `c` counts down
until it reaches zero at the right case. Lines like `L=t[q[E]]; L; r+=q;
L>>=q;` expand into a chain of 47 such tests, and `L+=L-=L=z*w;` is three
separate instructions (`t+=s`, `t-=s`, `t=z*w`). The extra `{ { } { ; } }`
inside the macro is balanced filler that does nothing.

5. A two register stack machine

The interpreter keeps the top two values of its stack in the globals `t` and
`s`. Instructions 5 to 15 load a value, so before them `s=t` moves the old top
down (`if(c>4&&16>c)s=t`). Each call also copies `t` and `s` into locals `T`
and `S` that the routine can read back later. Values are passed between
routines only through these registers, so for example the string `"Z"`
(push `S`) is a swap of the top two values and `"cr"` (subtract, not) tests
two values for equality.

6. Switch statements through a skip flag

The instruction `z` (`C=t--`) sets the global `C`, and while it is set every
instruction is skipped. Each `{` lowers the counter, and when it reaches
zero normal execution starts again, so `t` selects the n-th section after
the `z`. When `{` is not being skipped it returns from the routine, so it
works like `case n: ... return;`. Subroutine calls made while skipping
return right away because of the `if(!C)` test at the top of `main`.

7. Registers hidden below the program's memory

`m=256+Y` leaves 256 bytes in front of the address that the rest of the code
treats as 0. Small negative addresses are used as named variables: the
operands sit at `m[-14]` to `m[-7]`, `m[-6]` holds the opcode and then the
operand count, `m[-5]` is the status line flag, `m[-4]` is the output length,
`m[-1]` is the column and `m[-128]` is the output buffer. In the byte code
these are just numbers such as `TL` (push -4).

8. Filler and dead instructions

Many spaces in the strings are calls to `E[0]`, which only overwrites `t`
and `s`. They are placed where the next instructions overwrite those values
anyway, so they change nothing but help draw the picture. There is other
dead code as well, for example the `srand()` branch that can never run
because the value passed to it is never negative, and some steps before
`get_next_prop` whose results are thrown away.

9. Reusing storage for unrelated jobs

The game's stack is also scratch space for the interpreter's own temporary
values. The output buffer is used to collect the 5 bit codes of a word before
it is looked up in the dictionary. The program counter `x` is also the
pointer for reading strings, writing files and reading input, so it is saved
and restored with the same routines that are used for calls. Clearing the
screen stores 10 into memory location 0 and writes that byte 64 times.

10. Arithmetic tricks

The operand types of short instructions are made from the opcode with
`(opcode|15)`, then `+1`, `*4`, `-1`. The very first instruction run is the
number 183 (`T\377`), which is the opcode for restart, so starting the game
and restarting it use the same code. `x%y` is built as `x-(x/y)*y`, `~x` as
`-1-x`, and a 32 bit dictionary key is built five bits at a time with
shifts and adds.

11. Hidden text inside escape sequences and line continuations

Some strings are split across lines with a backslash, so `"T\2\` followed by
`50U..."` on the next line is really `"T\250U..."`. The data strings use
octal escapes like `"\0337\33[H"`, which are the terminal codes to save the
cursor and move it home.

12. Missing headers and mixed pointer types

There are no `#include` lines, so `open`, `read`, `write`, `creat`, `rand`
and `exit` are all implicitly declared. Pointers of type `unsigned char *`
are assigned to `unsigned short *` (`J=m+t`) without a cast, which relies on
compiler warnings being turned off.

13. Meaningless names

Every variable is a single letter (`t`, `s`, `x`, `y`, `k`, `J`, `E`, `C`,
`T`, `S`), and the routines have no names at all, only positions in `E[]`.
