# 2025/ncw1

Difficulty: Hard

1. Shaped source and broken up tokens

The code is squeezed into a block shape with runs of spaces in odd places,
so words such as `return`, `case 1` and `D[H+15]` are split over lines
(`return\n;`, `case 1\n:`, `D[65535]&D\n  [H+15]`).  The compiler ignores
the spacing, so simple.c uses normal indentation with one statement per
line.

2. One letter names for everything

Every function and variable is a single letter: `D` is memory, `t` the
CPU registers, `F` the program counter, `J` the stack pointer, `w` the
interrupt enable flag, `X` the halt flag, `T` the pending EI, `U` the
joypad, `I` reads memory, `N` writes memory, `Z`/`P` get and set 16-bit
register pairs, `W`/`V` get and set 8-bit registers, `L` sets the flags,
`Q` pushes, `j` is the ALU, `m` runs one instruction and `i` works out
the colour of one pixel.  Even `F`, which usually means "flags" on this
CPU, is the program counter while the flags are `t[6]`.  simple.c uses
names like `memory`, `pc`, `sp`, `read_byte()`, `push16()` and `alu()`.

3. Short macros that hide expressions

`#define G t[6]` and `#define A t[7]` name the flag and accumulator
registers, `E` and `B` test the zero and carry flags, `M` reads the HL
pair, `S` reads a 16-bit value after the opcode and `_(r)` reads two
bytes and moves a pointer, using the global `b` as scratch.  `O` and `Y`
are half of an `if` statement (`O<4)` becomes `if(y<4)` and `Y 4)`
becomes `if(y == 4)`), so the `if` keyword never appears where it is used.
simple.c replaces them with named functions such as `fetch16()` and
`pop16()` and normal `if` statements.

4. Compile time macros from the Makefile

Parts of the program are not in the source at all.  `K` (the keyboard
map), `R` (the starting register values) and `C` (the instruction fetch
`I(F++)`) are given with `-D` on the compiler command line, and the
headers are pulled in with `-include` instead of `#include`.  You have to
read the Makefile to know what `C` or `t[]=R` means.  simple.c includes
the headers itself, calls `fetch8()` for `C`, and gives default values
for `K` and `R`.

5. iso646.h operator words

`<iso646.h>` lets the code write `and`, `or`, `not_eq`, `or_eq`,
`and_eq` and `xor_eq` instead of `&&`, `||`, `!=`, `|=`, `&=` and `^=`.
They look like Python but they are mixed with the symbol forms (`&`, `|`
and `!`), which is confusing because `and` (logical) and `&` (bitwise) do
different things.  simple.c uses the normal C operators.

6. XOR used as "not equal"

Tests like `a^H?r:...`, `i^6?t[i]:...` and `if(o^7)` use `^`: two
numbers XORed together give zero only when they are equal, so `a^b` acts
like `a != b`.  `a^H+68` relies on `+` binding tighter than `^`, so it
means `a != 0xFF44`.  simple.c writes `!=` with named addresses such as
`IO_LY`.

7. Opcode decoding with bit fields

Instead of a 256 entry table, each opcode is split into the fields
`x=o>>6`, `y=o>>3&7`, `z=o&7`, `p=y>>1` and `q=y&1`, and nested
`switch` statements on these fields pick the instruction.  Many fields
are then used as numbers, for example `9-2*z` is +1 for INC and -1 for
DEC, `M+5-2*p` is HL+1 or HL-1, `F=y*8` is the RST address and `w or_eq p`
turns RETI into "RET and enable interrupts".  The table of conditions
`k[]={!E,E,!B,B}` is indexed with `y` or `y-4`.  simple.c keeps the same
decoding but comments each case with the instruction name (`JR cc,e`,
`LD (a16),SP`, `RST` ...) and uses `condition_met()` for the conditions.

8. One expression for many instructions

A single expression does the work of several instructions.  The ALU
`j()` decides ADD/ADC/SUB/SBC/AND/XOR/OR/CP from bits of `o` with
`o&1 and o<4` (use carry), `o>1` (subtract) and `o&4 and o^7` (logical).
The CB shift code `c=y not_eq 6&v>>7* !(y&1)` depends on the precedence
order `*`, `>>`, `!=`, `&`, and means "the bit shifted out, unless this is
SWAP".  simple.c spells these out with named ALU operations and separate
`if` statements for the carry and the rotated-in bit.

9. Comma operators and nested ternaries instead of statements

Many statements are joined into expressions with `,` and `?:`, for
example the scan line counter
`++*a==144?D[H+15]or_eq 1,q=0:*a>153?*a=q=0:0`, the key handling
`d[f]?d[f]--,U xor_eq 1<<f:0` and the interrupt
`w and(...)?w=0,D[H+15]and_eq~1,Q(F),F=64:0`.  `P()` uses
`k<3?(...):p?(...):(J=v)` to pick between three register sets.
simple.c turns these into `if`/`else` blocks.

10. Magic numbers and pointer offsets

Hardware addresses are written as decimal numbers or as offsets from
`H` (65280, which is 0xFF00): `H+4` is the DIV register, `H+15` is IF,
`H+64` is LCDC, `H+66` is SCY, `65024` is the sprite table, `38912` the
tile map and `65535` the interrupt enable register.  In `main()` the
pointer `a` points at LY (0xFF44), so `a[-4]` is LCDC and `a[187]` is
0xFFFF.  simple.c gives each one a name such as `IO_LCDC`, `OAM` and
`TILE_MAP`.

11. Implicit int and type truncation

No function has a return type or parameter types, so they all default to
`int`, and functions that return nothing are still `int`.  The program
relies on `unsigned char` variables to cut values to 8 bits, for example
`o=W(y)+9-2*z` wraps INC/DEC and `x=r` keeps only the low byte for the
zero flag in the CB instructions.  simple.c uses `void` where nothing is
returned and makes the 8-bit truncation visible with a `u8` variable or a
`(u8)` cast.

12. Reused variables

Variables are reused for unrelated jobs.  In `m()` the opcode fields `o`,
`x`, `y` and `z` are overwritten by the CB prefix byte, and `x` is then
reused again to hold the result.  In `main()` `p` counts CPU cycles, `q`
remembers if the frame was drawn and `e` is the output pointer, while in
`i()` the same letters mean the pixel position and the LCD control
register.  simple.c moves the CB instructions into their own function
and gives each value its own name.

13. File descriptor 3 instead of opening a file

The ROM is read with `read(3,D,32768)` and there is no `fopen` or `argv`.
The shell opens the ROM file on descriptor 3 (`./prog 3< rom_file`), so
the code looks like it reads from nowhere.  simple.c keeps this but
explains it in a comment.
