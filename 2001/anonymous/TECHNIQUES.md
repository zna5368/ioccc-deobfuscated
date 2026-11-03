# 2001/anonymous

Difficulty: Catastrophic

1. One letter macros for types, keywords and expressions

`#define l int*`, `#define F char` and `#define c return` hide types and
keywords. Bigger pieces of logic also hide behind single letters: `G` reads
the next signed byte of x86 code, `I` reads the next 32-bit word, `g` is the
"reg" field of the ModRM byte (`y/010&7`), `R` is the register array
(`B+13`), `U` pushes onto the x86 stack, `O` pops from it and `C(O,D)` reads
field `D` of section header `O`. Lines like `l B,i,n,a,r,y,P;` look like they
declare seven pointers, but only `B` is an `int*`; the rest are plain `int`s.

2. Macros that are only used inside strings

Many of the macros (`R`, `U`, `O`, `E`, `L`, `V`, `K`, `P`) are written into
`sprintf` format strings such as `"U R[%d],"` or `"*L(V(%d+%d)+%d,0)"`. The
program writes C code with them and compiles that code later, so a
preprocessor or beautifier cannot expand them. In `simple.c` the generated
code uses readable names: `PUSH(...)`, `POP()`, `REG[...]`, `le32()` and
`guest_ptr()`.

3. Macros given only on the compiler command line

The Makefile passes `-DA(X)=#X` (turn the argument into a string),
`-DX=A(<compiler command>)` and an empty `-Dmagic=`. None of these appear in
the source, so `A(.strtab)`, `A(main)` and `A(rm -f .%s)` look like broken
code but are really the strings `".strtab"`, `"main"` and `"rm -f .%s"`.
`-Dprocessor` together with `#define processor x86` causes the "useful"
warning `"processor" redefined`, and `x86` itself is
`(F*)index(ss+V(i),0100)`, a search for the `@` in a symbol name like
`printf@GLIBC_2.0`.

4. A program that recompiles itself

When a basic block of x86 code has been translated, `p()` calls `system()`
to run the compiler on `__FILE__` (the program's own source) with
`-Dmagic='<translated C code>A'`, and the next step `execv()`s the new
program. `stat()` on the program name `.<file><address>` is a cache check:
if the block was already compiled it is run straight away. The x86
registers survive between these programs because they are stored inside the
x86 ELF file itself (`R` is `B+13`, just after the 52 byte ELF header) and
the file is mapped shared and writable with `mmap`.

5. Turning code into a string

The start-up code is wrapped in `magic((...))`. In the first build `magic`
is empty, so the code runs. In a recompiled program `magic` is the
translated code followed by `A`, so the line becomes
`<translated code>,A((start-up code))`, and `A` turns the start-up code
into a string literal that does nothing. `simple.c` keeps this idea with
`TRANSLATED_BLOCK(start_at_main(size))` and `AS_STRING`.

6. Function pairs made with `##` and shadowed globals

`Tr(an,sl,at,or)` builds two functions, `an` and `an##i`, and
`add(Ev,Gv)` builds `Ev` and `Ev##n`. `Tr(L,...)` makes `L`/`Li` (turn an
x86 address into a host pointer) and `Tr(Run,...)` makes `Run`/`Runi` (find
the symbol and string tables). The functions have old style parameters named
`d`, `r` or `a`, the same names as global variables. In `L` the parameter
`r` hides the global `r`, so `L` changes nothing outside, while `Run` uses
`a` as its parameter and so changes the global `r`. `add(Sca,...)` and
`add(ru,...)` make `Scan`/`Sca` and `run`/`ru`, which search the symbol
table for an address or for `main`.

7. Recursion instead of loops

`Li`, `Runi`, `Sca` and `ru` call themselves to step backwards through
section headers or symbols, ending the recursion with `?:` or `&&`. In
`simple.c` they are `do`/`while` loops.

8. Picking format strings with pointer arithmetic

Several format strings are packed into one string literal, separated by
`\0` and padded with spaces, and the right one is chosen by adding an
offset worked out from the opcode. `"...R[%d]=E(~-E(R[%d])),\0 ... U
R[%d],\0 ..."+(a&'8')*3` uses bits 3-5 of the opcode to choose between
decrement, push, pop, push-immediate and "set to 0". Others are
`"..."-a%'w'%'j'*5` for `mov`/`lea`/`test`, `a%' '+29+"P O,\0 U %d,P=%d,"`
for `ret`/`call`/`jmp`, `"+    -  "[g]` for add or subtract and
`"& "[a%3%2+1]` to put `&` in front of the operand for `lea`. Because the
formats share one argument list, some of the generated code contains useless
numbers like `3,` or `5,32,` that are thrown away by the comma operator.

9. Opcode tests hidden as arithmetic

The opcode `a` is a signed byte, so `a+127`, `a+'}'`, `a+1`, `a+61`,
`a+24`, `a+025`, `a+'H'` and `a+'{'` are zero exactly for the opcodes
0x81, 0x83, 0xff, 0xc3, 0xe8, 0xeb, 0xb8 and 0x85. `G-'u'` checks for a
`jne` (0x75), `a%061<=0` keeps the ModRM byte for `xor` (0x31) and for all
negative opcodes, and `y/'@'-3` tests the ModRM "mod" field. Numbers are
written in octal or as characters (`010`, `014`, `021`, `0100`, `0377`,
`'\r'` for 13, `'8'` for 0x38) or in decimal (`65280` for 0xff00).

10. The whole program in one expression

`pain()` is a single `return` statement built from nested `?:` and comma
operators, several hundred characters long. It decides between first time
start-up and translating one instruction, and every instruction class is
another branch of the same expression. `-~i` (i+1) and `~-x` (x-1) are used
instead of `+1` and `-1`.

11. Endianness detection with a pointer

`z=(char*)&ck` points at the first byte of a pointer variable holding the
argument count. That byte is non-zero only on a little-endian machine, so
`E()` (which swaps the byte order of a word) leaves values alone when `*z`
is set. `z` being non-NULL is also used as the flag that start-up has
already been done.

12. Misleading names

`pain` does the main work, `ck`/`cka` are the argument count, `magic` is the
translated code, `processor`/`x86` find an `@` in a string, and the shell
variable named `warning` only exists to produce a compiler warning.
