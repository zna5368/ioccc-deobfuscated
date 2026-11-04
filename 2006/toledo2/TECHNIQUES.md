# 2006/toledo2

Difficulty: Terrifying

1. Whole program inside a `for` statement

All the work happens in the header of one `for` loop that has an empty body
(`;`). The first part opens every file, sets up the terminal and loads memory.
The test part `118-(x=*c++)` fetches the next opcode and stops at `0x76` (the
8080 `HLT` instruction). The "increment" part is one huge expression that runs
the instruction. A normal loop would put the instruction code in the body.

2. Giant nested ternary instead of `switch`

The opcode is decoded by a chain of about fifty `?:` operators. The chain uses
a running subtraction: `z=(x&199)-4 S 1 S 1 S 186 ...` where `S` is
`)?(z-=`. Each step subtracts the gap to the next opcode, so `z` reaches zero
exactly when the opcode matches, and the zero picks the "false" branch. The
branches are written at the end in reverse order, separated by the macro `O`
(`)):((`), so the code for an opcode is far away from the test that selects
it. `C` (`S 5 S 3`) repeats the gap pattern of the `0x02..0x3F` single-byte
opcodes, and the stray `+129` that follows turns the last gap into the jump to
`0xC3` (`JMP`).

3. Macros that hide expressions and unbalanced parentheses

Short one-letter macros stand for whole operations: `Q` is the accumulator,
`s` is the flags, `p` pops a word, `w` pushes a word, `q` reads a 16-bit
immediate, `m` fetches an ALU operand, `d(e)` builds a register pair and
`a(f)` returns a register or memory at `HL`. Several macros (`n`, `b`, `S`,
`O`, `D`) leave parentheses or a `?` open, so they only make sense when
combined with the text that follows them in the source.

4. Register and memory selection by arithmetic

`a(f)` is `*((7&f)-6?&o[f&7]:&l[d(5)])`: the low three bits of the opcode are
used directly as an index into the register array `o[]`, and code 6 means
"memory at HL" instead. The register pair index `r=(y>5)*2+y` maps pair codes
to array slots so that the stack pointer lands in `o[8]`/`o[9]`, and the same
`r` points at the low byte for `DAD`/`DCX` and at the high byte for
`LXI`/`INX`/`PUSH`/`POP`.

5. Condition codes from a string table

The macro `b` tests a jump/call/return condition with
`(y&1?~s:s)>>"\6\0\2\7"[y/2]&1`. The string literal is really a small array
holding the bit numbers of the Zero, Carry, Parity and Sign flags, and the low
bit of the condition picks "flag set" or "flag clear" by inverting the flags.

6. Bit tricks for flags

The macro `h` computes all flags in one line. The carry comes from bit 8 of a
wide result, auxiliary carry from bit 4 of a separate low-nibble sum (`z`),
and parity is folded with `y^=y>>4,y^=y<<2,y^=~y>>1`, which leaves "even
parity" in bit 2 where the 8080 keeps it. `INR`/`DCR` keep the old carry by
passing `s<<8` so that `>>8&1` gives the old carry bit back.

7. Shared generic add/subtract macro

`n(o,p,e)` builds `ADD`, `ADC`, `SUB`, `SBB`, `CMP`, `INR` and `DCR` from one
template by passing the operator (`+` or `-`) and the carry-in as macro
arguments. `INR`/`DCR` set `x=0` and pass `1` as the "carry" so the same code
adds or subtracts one.

8. Function pointer cast to choose read or write

The disk command uses `((int (*)())(2-*c++?fwrite:fread))(...)` to pick the C
library function at run time and call it with the same arguments. The `fseek`
to the sector is hidden inside the last argument as `(fseek(...),e)`, and the
`whence` argument `Q=0` also clears the accumulator.

9. Nested `fopen` calls with side effects

`v A((u A((e A((r-2?0:(V A(1[U])),"C")),...,"B")),"A"))` opens the import
file, the boot image, the two drives, runs `stty` and reads the boot image all
inside the arguments of `fopen`. The comma operator makes each inner
expression evaluate to the next file name. The closing `fclose` calls are
nested the same way.

10. Reversed array indexing

Expressions like `9[o]`, `7[o]`, `1[U]`, `5[k]` and `q[l]` use the rule that
`a[b]` is the same as `b[a]` to make indexing look strange. `q[l]` even puts a
whole immediate fetch expression in front of the brackets.

11. Magic numbers

Opcodes, masks and addresses are written in decimal: `118` is `HLT`, `199` and
`207` are the masks `0xC7` and `0xCF`, `237` is the `0xED` disk prefix,
`64506` is the disk parameter block at `0xFBFA` and `~40` clears flag bits 3
and 5.

12. Decorative brace art and old-style `main`

The block of empty braces drawn in the shape of `8080` is only nested empty
compound statements (with two `;` empty statements) and does nothing.
`main(r,U)char**U;` uses an old K&R definition with an implicit `int`
argument, and `r` (argc) is reused later as a scratch variable.
