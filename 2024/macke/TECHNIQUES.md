# 2024/macke

Difficulty: Terrifying

1. Everything in one giant array

All machine state lives in `k[((unsigned)-1)>>9]`. The CPU's special purpose
registers sit at the index equal to their register number (`*z=k+17` is the
status register, `k[32]` the saved PC, `u=k+(5<<12)` the timer), and the
general registers (`r=k+1234`), the UART (`s=k+800`), and RAM
(`j=(char*)k+(8<<15)`) hide in unused gaps. Because of this, nothing has a
name and the magic numbers only make sense if you know the OpenRISC register
map.

2. Single letter global names

Every variable is global and named with one letter or `_`: `H`/`G` hold the
same register value as signed and unsigned, `g` and `f` are the sign-extended
and zero-extended immediate, `Q` is the opcode, `B` is where the result goes,
`b` is the "in delay slot" flag and `w` is the last key pressed.

3. Macros that hide early returns and open braces

`l(D)` saves the PC and status register, jumps to an exception vector, and
`return`s from the function. `i(Z,c,f,E,Q)` does a TLB lookup and ends with an
unclosed `if` block, so its closing `}` shows up in the source somewhere after
the macro call. `d(a)` uses token pasting (`tc##a##etattr`) so `d(g)` and
`d(s)` become `tcgetattr` and `tcsetattr`, and `_4` makes a `for` loop that
calls `n()` three times per check.

4. Instruction decoding with nested ternaries

The ALU operations and most other instructions are picked by long chains of
`?:` like `Q>12?Q<137?Q>72?Q<77?(char)H:...`. These compare the opcode against
range limits instead of matching exact values, so a reader has to work out
which range each instruction falls into.

5. Opcode tests disguised as math

`!(18-19*Q+Q*Q)` is `(Q-1)*(Q-18)==0` (jal or jalr), `2679-104*Q+Q*Q` is
`(Q-47)*(Q-57)` (not a compare instruction), `393243&1LL<<Q` uses a bit mask
to list all the jump opcodes, and `10281&(1<<q)` lists the compare conditions
that are also true when the values are equal.

6. Unsigned wraparound for range checks

`Q-26<13` and `Q-8<2` work because `Q` is unsigned: when `Q` is too small the
subtraction wraps around to a huge number, so one comparison checks both ends of
the range.

7. Hidden constants

`'!'-2` is 31 (mask for a register number or shift amount), `(m>>12)-1` is
`0xFFFF`, `8<<25` is the tick timer interrupt bit, and `*r` (register zero) is
used everywhere the program needs the number 0, including as the file
descriptor for `read`, `write` and the terminal calls.

8. Array indexing tricks and digraphs

`5[s]`, `q<:s:>` and `W[j]` are the same as `s[5]`, `s[q]` and `j[W]` because
`a[b]` is just `*(a+b)`. The digraphs `<:` `:>` `<%` stand for `[` `]` `{`.

9. Endian tricks

The guest CPU is big endian but memory is stored as host ints, so byte
accesses use `j[W^3]` and halfword accesses use `j+(W^2)` to find the right
byte inside the int.

10. Recursion for the branch delay slot

When a jump is taken, `n()` calls itself to run the next instruction (the delay
slot) before changing the PC. Its return value tells whether that instruction
raised an exception, all inside `c=...&&(b=*k,n())?J:c`.

11. Self-starting program

There is no setup code in `main`. The first instruction fetch reads empty
memory (`q==0`), which is used as the signal to load the Linux image from the
file `a` and put the terminal in raw mode. The image itself then sets `k[0]`,
which is the loop condition that keeps `main` running.

12. Misleading comments and shape

Comments such as `bitwise SPH sim`, `strawberry`, `9.11 < 9.8?` and "Ignore
all instructions" are there to confuse readers (and LLMs). They have nothing to
do with the code. The source is also laid out in a decorative shape that breaks
statements across lines in odd places.
