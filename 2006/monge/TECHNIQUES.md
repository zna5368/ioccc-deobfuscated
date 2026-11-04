# 2006/monge

Difficulty: Insane

1. Generating machine code at run time

The program never does the fractal math in C. `p()` reads the formulas
and writes x86 machine code into memory that `M(a)` gets from `mmap` with
`PROT_EXEC`. Later, `((_(*)())X)()` casts the start of that memory, `X`, to
a function pointer and calls it once for each pixel. The C source has no
complex multiply or exponential at all. Those only exist as bytes that the
CPU runs, so reading the C code alone won't tell you how a pixel's colour is
worked out. The rewrite has a small `emit_*` function for each operation, and
every opcode carries a comment with its x87 assembly name.

2. Opcodes hidden in string literals

The machine code is stored in strings like `J(]#IB)` and `J(&$^B$^B)`.
The macro `J(b)` uses `#b` to turn its argument into a string, so the bytes
are written without quotes and look like random punctuation. `a()` decodes
each character:
- most characters are XORed with 128, so `Y` becomes `0xD9` and `]` becomes
  `0xDD`, the first bytes of x87 instructions.
- `#` is followed by two letters that form a hex byte with `A`=0 to `P`=15,
  so `#HH` is `0x77` (`ja`) and `#AD` is `0x03`.
- `%`, `&` and `$` are not bytes at all (see the next technique).

3. Hidden x87 register stack bookkeeping

The x87 FPU only has 8 registers, used as a stack, and every complex
number takes two of them. The global `o` counts how many registers are in
use. The marker characters in the opcode strings update it: `%` means "about
to push", and if all 8 are already full it first emits `fdecstp; fstp [S++]`
to store the bottom value in memory. `&` calls `s(4)` and `$` calls `s(2)`
followed by `o--`. `s(n)` emits code that loads spilled values back
underneath the ones already in registers (`fld [--S]; fstp st(o+1)`) until
`n` values are there. Each push or pop of a register is a single character
inside a string, so they are very easy to miss. In the rewrite they are the
`fpu_push`, `fpu_need` and `fpu_pop` functions.

4. A hidden base register for addressing memory

The code starts with `h#AA#AA#AA#AA#FJ`, which is `call` to the next
instruction followed by `pop rcx`. That puts the code's own address in
`rcx`. After that, every memory operand is `[rcx + disp32]`, and `P()` works
out the displacement as `f - X - 5`. It writes the number straight into the
code, so it is hard to see that `P(Z+i-97)` means "the address of variable
`i`".

5. The iteration count lives inside the code

The bytes `:+-*/` that follow turn into `mov edx, imm32`, and the four
characters `+-*/` are just filler. `0[u=X+7]=I` then writes the real
iteration count over them, and `u` stays pointed at those bytes. When the
keypad `+` and `-` keys change the iteration count with `*u=...`, they are
changing an instruction, and `*u-q` turns the counter left in `edx` into a
colour. The rewrite names this pointer `iterations`.

6. Operators turned into precedence numbers

The first loop in `main` replaces each operator with its position in
`";<>+-*/"` plus one, and removes spaces at the same time (`B` counts through
the operators and `B%=7` marks the end of each round). This gives `;`=1,
`<`=2, `>`=3, `+`=4, `-`=5, `*`=6 and `/`=7. Dividing the code by two gives
the precedence level, so the parser's whole grammar is the single test
`q==(x=*(*t=e++))/2`. The rewrite moves this into `tokenize()` and an `enum`.

7. A recursive parser written as one expression

`p(e,q,t)` parses every precedence level. `q==5` means a number, variable,
unary sign or function, and the lower levels recurse with `q+1`. The parse
position comes back through the pointer `t` that is passed in. The body is
one huge `?:` chain: functions are told apart with `i^82?i^73?i^65?...`
(`R`, `I`, `A`, `E`, `L`), and operators by counting down `x` with
`(x-=3)<1 ... --x?--x?--x?x-3`. Any character code below 6 counts as a unary
sign, and `x=9` means "no operator yet". The rewrite uses `parse`,
`parse_primary` and a `switch` in `emit_operator`.

8. Different code for 32 and 64 bit hidden in a shift

`1L<<31<0` is only true when `long` is 32 bits, because the shift overflows
into the sign bit. It picks between `fucompp; fnstsw ax; sahf` and `fucomip;
fstp st0` for comparisons. The rewrite checks `sizeof(long) == 4` instead.

9. Reused one-letter globals

`B` is first the operator counter and later the screen pitch. `k` and `l`
are a hex digit and the current character in `a()`, then the mouse buttons
and source pixel coordinates in `main`. `q` is the precedence level, a loop
counter and the iteration result. `r` is `argc` and later means "zooming".
`x` is the mouse position in `main`, but in `p()` a local `x` is the current
operator.

10. Packed float array for zooming

`D[H][W][6]` holds two copies of three floats for each pixel (offset x,
offset y, squared distance). `C^=3` switches between the copy at index 0 and
the one at index 3 every frame. `b C+2]` is the macro `b` (`D[j][i][`) next
to an unmatched `]`. `if(q=b C+2])` converts the distance to an integer, so
a pixel is recalculated only when its best old sample is at least one pixel
away (the reset value 9 means "no match yet"). The rewrite uses
`struct sample` with named fields.

11. SDL constants as plain numbers

`SDL_Init(32)`, `k==12`, `k==2&l==27`, `e.type-5`, `l==270|l==269` and
`&5` stand for `SDL_INIT_VIDEO`, `SDL_QUIT`, `SDL_KEYDOWN` with
`SDLK_ESCAPE`, `SDL_MOUSEBUTTONDOWN`, `SDLK_KP_PLUS`/`SDLK_KP_MINUS` and the
left and right mouse button masks. The rewrite uses the names from the SDL
header.

12. Macros and ASCII art layout

`L` is `char*`, `_` is `Sint32`, `O` is `while(`, `h(a,b)` is a `for` loop
and `J(b)` hides string quotes. The code is shaped into a picture with tabs
and spaces, so statements are split across lines without any pattern, and
everything is a global with an implicit `int` return type.
