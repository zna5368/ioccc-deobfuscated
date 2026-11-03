# 1984/mullender

Difficulty: Terrifying

1. Machine code hidden in a data array called main

`short main[] = { 277, 04735, ... };` is not a function at all. It is an array
of numbers. The linker only looks for a symbol called `main` and does not care
whether it is code or data, so the C startup code jumps straight into the
array. The numbers are the raw bytes of machine instructions, so the CPU runs
them as a program. There is no C logic to read: you have to split each `short`
into two bytes (low byte first) and disassemble them by hand. On modern
systems the data area is not executable, which is why the original program
crashes with a segmentation fault.

2. One array that is two programs (PDP-11 and VAX polyglot)

The same bytes are valid for two different CPUs. Word 0 (`277`, octal `0425`)
is a PDP-11 branch instruction that jumps forward 21 words to the PDP-11 code
at word 22. On the VAX, `main` is entered with the `calls` instruction, which
treats the first word as a list of registers to save and starts executing at
word 1, so the VAX never sees the branch. Words 1 to 17 are VAX instructions
(`pushl $9`, `pushal str`, `pushl $1`, `calls $3, write`, `cvtwl $32767, r2`,
`decl r2`, `jneq`, `brb`) and words 22 to 37 are PDP-11 instructions
(`mov pc,r4`, `sys 4`, `sob r2`, `br`).

3. Shared string hidden inside the instructions

The message `"  :-)\b\b\b\b"` never appears as a string. Its 9 bytes start in
the upper half of word 17 (`020004` is the VAX `ret` byte `04` followed by a
space `0x20`) and continue through words 18 to 21 (`14880` is `" :"`, `10541`
is `"-)"`, and `2056`/`04010` are pairs of backspaces). Both the VAX and the
PDP-11 code point at the same bytes. The PDP-11 code finds the string without
any address by reading the program counter and subtracting from it
(`mov pc,r4; tst -(r4); sub $9,r4`).

4. Direct system calls instead of library functions

Neither version calls the C library. The VAX code carries its own copy of the
`write()` stub (`chmk $4; ret`, words 15 to 17) and the PDP-11 code traps into
the kernel with `sys 4` followed by the buffer address and length inline in
the instruction stream (words 30 to 32). The PDP-11 delay loop even calls a
system call that does not exist (`sys 55`, word 35) just to waste time.

5. Mixed number formats

Each number was written in a randomly chosen style: decimal (`277`), negative
decimal (`-4129` is really `0xefdf`), octal (`04735`), hexadecimal (`0xbef`)
or a character constant (`','`, `'#'`, `'p'`, `'r'`, `'e'`). They all just
mean a 16 bit value, but the mix hides patterns such as the repeated
backspace pairs (`2056` and `04010` are the same value `0x0808`) and the
readable text at the end of the array.

6. Leftover object file junk

Words 38 to 80 are not executed at all. They are the relocation entries and
symbol table copied out of the assembled object file, including the symbol
names `vax`, `pdp`, `str` and `write` stored as numbers like `0x6176` (`"va"`)
and `29303` (`"wr"`). They look like part of the program but only add noise.
