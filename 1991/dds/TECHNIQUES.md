# 1991/dds

Difficulty: hard

1. Program written for a made-up virtual machine

The C code does not contain the BASIC compiler at all. `main` is only a small
interpreter: `for(p=s;;p++)switch(*p++)` reads one letter from the string `s`
and runs the matching `case`. The real compiler is a program for the seven
instruction `COGNIMP$` machine stored in `s`. To understand the compiler you
first have to work out what each opcode letter (`C`, `O`, `G`, `N`, `I`, `M`,
`P`) does and then "disassemble" the string by hand. In `simple.c` the program
is split into one instruction per line with comments, and every `case` is
labelled with the instruction name.

2. Encoded string

Every visible character of `s` has been moved up by one (`P` is stored as
`Q`, `int` as `jou`, `;` as `<`). The loop `for(q=s;*q;q++)*q>' '&&(*q)--;`
subtracts one from every character above space when the program starts, so
the string can only be read after running that loop in your head. Spaces and
newlines are left alone, which makes the encoded text look even stranger.
`simple.c` stores the decoded text directly.

3. Several strings packed into one, found by pointer arithmetic

The end of `s` also holds the compile command and the output file name. The
loop from technique 2 leaves `q` pointing at the end of the string, so
`q-3` is `"a.c"` (passed to `fopen`) and `q-87` is the `cc ...` command
(passed to `system`). The two strings even share their last three characters.
`simple.c` uses the named constants `OUTPUT_FILE` and `COMPILE_COMMAND`.

4. Macros that hide half a statement

`#define Q r=R[*p++-'0'];while(` opens a `while` loop whose condition and
body are written after the macro, for example `Q*r)fputc(*r++,o);`. `#define
B ;break;case` ends one `case` and starts the next, so the `switch` has no
visible `break` or `case` keywords. The very first `B` even creates an empty
statement `;break;` before the first `case`.

5. Hidden program counter changes

The interpreter moves its pointer `p` in many places: `switch(*p++)` skips
the opcode, the `Q` macro skips the register digit, and the `for` loop's
`p++` skips the last operand. Some instructions cancel one of these with
`p--` (Output) or deliberately leave `p` on the character that the `for`
loop will skip (labels, Goto). Any letter that is not an instruction, such as
the `$` label marker, falls through the `switch` and is skipped together with
the next character. `simple.c` advances its pointer explicitly inside each
instruction.

6. `goto` into the middle of another case

The `If` case jumps with `goto G` into the `Goto` case to share the label
search loop. `simple.c` puts the search into a `find_label` function that both
instructions call.

7. Generated code is obfuscated too

The C program written to `a.c` is one long `switch` on the line number `l`
with no `break` between ordinary statements, a `default:` case that counts
`l` upward until a real line is found, an array `r[]` used as the GOSUB
return stack and arrays `f[]`/`m[]` indexed by a variable's letter to store
the FOR loop start and limit.

8. Short meaningless names and K&R style

Variables like `s`, `k`, `R`, `p`, `r`, `q`, `i` and `o` give no hint of what
they hold (the program, the last input character, the registers, the program
counter, a register pointer, the end of the string, the input file and the
output file). `main(c,v)char**v;` uses old style parameter declarations and
an implicit `int` return type.
