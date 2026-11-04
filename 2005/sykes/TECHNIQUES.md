# 2005/sykes

Difficulty: Insane

1. Single-letter macros that hide the C syntax

The top of sykes.c defines macros like `P` (`*p`), `_` (`m[n&`), `L` (`R(`),
`b` (`):L`), `G` (`):(`) and `r` (`=t&`). Most of them are only pieces of an
expression, often with unbalanced brackets, so a line such as
`L a=x)G L x=k)G` only makes sense after the preprocessor glues the pieces
together. `F` (`_++k+O]`) is a stack pull and `Y` (`_ k--+O]=`) is a stack
push. simple.c expands them and uses `PUSH()`, `PULL()` and `READ()` macros
with real names.

2. Macros that expand to long chains of `t--?`

`W` is `t--?t--?` and `M` is seven `W`s, so `M M M W` gives 44 copies of
`t--?`. Each `t--?` subtracts one from `t` and goes on to the next test while
`t` is not zero. When `t` reaches zero the matching alternative after the
`:` signs is chosen. This turns a number into a choice between 44
operations, like a `switch` statement written as one giant expression.
The addressing modes use a shorter chain (`W W W W`, 8 tests). simple.c uses
two real `switch` statements.

3. The whole CPU in one expression

Line 56 holds 64 `?:` operators after preprocessing. Every 6502 instruction
(ADC, AND, ASL, branches, BIT, BRK, ...) is one alternative, written with
the comma operator so several statements fit where one value is expected,
for example `t=a+P+C,C r O?1:0,V=...,L a r f)` is ADC. The alternatives are
listed backwards, so the last one in the source (ADC) is number 0. simple.c
gives each instruction its own `case`.

4. One string used as four lookup tables

The string `j` is 528 bytes long and contains four tables: bytes 0-127 give
the PET keyboard row and column for each ASCII key, bytes 128-255 say if
SHIFT is needed, bytes 256-511 give the operation for each opcode (stored as
`'#'` + operation number) and the last 16 bytes give the addressing mode as a
digit. The tables are read with `j[h]`, `j[h|l]` (`l` is 128), `j[i+O]` (`O`
is 256) and `j[...|O+O]`. simple.c uses four separate arrays, with the
opcode table laid out as a 16 x 16 grid of instruction names.

5. Keyboard positions packed into printable characters

Each key is stored as the character `(row + 5) * 8 + column`, so `j[h]/8-5`
is the row and `j[h]%8` is the bit in that row. Adding 5 rows makes every
entry a printable character, and a space (32) gives row -1 which never
matches. simple.c uses a `KEY(row, column)` macro with designated
initializers like `['A'] = KEY(4, 0)`.

6. fopen() modes hidden inside the table

`fopen(..., j+32)` and `fopen(p, t?j+61:j+32)` pass pointers into the middle
of the table as the file mode. `j+32` starts with `"r(0)1*+"`, which the C
library reads as `"r+"` (it ignores the unknown characters but sees the `+`).
`j+61` starts with `"w"`. These letters are also the keyboard entries for the
space and `=` keys. simple.c writes `"r+"` and `"w"` directly.

7. Numbers written as other numbers

`O` is 256, `n` is 65535, `l` is 128 and `f` is 255, and they are combined
to write addresses: `l-9` is `$77` (BASIC text pointer), `n-1` is the IRQ
vector `$FFFE`, `n-3` is the reset vector `$FFFC`, `O+O` is the input buffer
`$200`, `e^O*O/2` is `e - $8000` for screen addresses, `59408` and `59410`
are the PIA ports `$E810` and `$E812`, `34` is `'"'` and `96` is the RTS
opcode. simple.c gives every one of these a named constant.

8. Reusing variables for unrelated jobs

`c` (argc) becomes the interrupt period, `i` is first the load address
`$C000`, then the current opcode and then `A_REVERSE`, `t` and `s` are used
as temporary values everywhere, `p` points at the operand but also at the
file name and at BASIC pointers, and `g` is the program file and later the
LOAD/SAVE file. simple.c gives each job its own named variable.

9. Hidden kernal LOAD/SAVE trap

`t=d%65493` (65493 is `$FFD5`) followed by `if(w+t<4)` catches the program
counter reaching the PET kernal LOAD (`$FFD5`) and SAVE (`$FFD8`) entry
points. `w` is the end address of the loaded file and is 0 only for a 16384
byte PET ROM, so the trap only happens in PET mode. The trap reads or writes
the file named in quotes and then replaces the opcode with `96` (RTS) to
return to BASIC. The SAVE loop `t=_++i]?3:t-1` counts down three zero bytes
in a row to find the end of the program. simple.c moves this into
`kernal_load_save()`.

10. Flags stored as separate bit values

Each status flag lives in its own variable holding its own bit value (`Z` is
0 or 2, `I` is 0 or 4, `V` is 0 or 64, `S` is 0 or 128), so the status byte
is just `C|Z|I|D|B|V|S|32` (macro `q`). The macros `E` and `J` pick a flag
from the top two opcode bits (`s=i>>6`) for the branch and set/clear flag
instructions. simple.c uses named flags with `branch_flag()` and
`flag_to_change()` helpers.

11. Screen output as a side effect of every instruction

There is no explicit "write to screen" code. After every instruction,
`t=e^O*O/2; if(t<1000)` checks if the operand address was in screen memory
(`$8000` to `$83E7`) and redraws that cell with `mvaddch()`, using reverse
video when bit 7 is set. The expression `s+w<32?s+64:s>95+w?s-32:s` converts
PET screen codes to ASCII only in PET mode, because `w` is large otherwise.

12. Code drawn as a picture

The `main()` function is formatted into the shape of a PET computer, with the
comments `### COMMODORE BASIC ###` and `READY.` placed on its screen. The
spacing splits tokens and expressions over many lines, so the code has to be
reformatted before it can be read.
