# 1996/august

Difficulty: Terrifying

1. A whole program hidden inside a comment

The big comment at the top of `august.c` looks like random whitespace and
garbage, but it is a compressed copy of the bytecode for an OC compiler. The
interpreter does not read it from its own source code at compile time. Instead,
you run `cat august.c program.oc | ./august`, so the interpreter reads its own
source file from standard input, unpacks the comment as a program, skips the
rest of the C code (up to the form feed `\f` at the very end of the file) and
then runs the compiler on `program.oc`.

2. Compression that hides bytes in whitespace

The IOCCC size rules do not count whitespace or `;`, `{` and `}` when they are
followed by whitespace. The bytecode is packed so that these "free"
characters carry most of the data. A `;`, `{` or `}` starts a back reference
(copy 2, 5 or 8 bytes, plus 0 to 3 more, from earlier in the output, like LZ
compression). A space, tab or newline followed by one or two more of these
characters picks a byte out of the lookup table string
`d=" \t\n;{}\0A..."`. The index is worked out with tricks like `o%4` and
`(r+(r==123))%8*3`, which turn each character into a small number. Any other
character is copied as it is.

3. Keywords and code hidden in the Makefile

Many important parts of the program are not in `august.c` at all. The
`Makefile` passes `-D` options such as `-DI=if`, `-DE=else`, `-DW=while`,
`-D'g=getchar()'` and `-D'T=m[s]'`. The macro `D` holds nine whole cases of
the interpreter's big `if`/`else` chain (subtract, multiply, divide, compare,
assign, ...), and `R` holds the code that pops the arguments after a function
call. Reading only `august.c` you cannot tell what `I(o)`, `W(p)` or `D` mean.

4. Macros that build an `if`/`else if` chain

`C(l,s)` expands to `if(o==l){s;}else`. Writing `C(59,r=2)C('{',r=5)...r=0;`
gives a long `if ... else if ... else` chain without ever writing those words.
`Q(l,o)`, `O(l,o)` and `H(l,s)` are built on top of `C` for binary operators,
unary operators and "push a value" opcodes, so a single short line such as
`Q(12,+)` is really "if the opcode is 12, pop a value and add it to the top of
the stack". In `simple.c` these chains are written as `switch` statements.

5. Unbalanced macro that hides a bracket

`X` is defined as `m[s-`, so `X 1]` means `m[s-1]`. The opening `[` lives in
the macro and the closing `]` is in the code, so the brackets never look
matched. `X -l/2]` becomes `m[s- -l/2]`, which is a hidden way to write
`m[s+l/2]`.

6. One array used for everything

The single array `m` holds the bytecode, the global variables and the stack
(which starts at the end of the array and grows downwards). Single letter
variables are the virtual machine's registers: `p` is the program counter, `s`
the stack pointer, `f` the frame pointer and `c` the code size (where globals
begin). `p` is also reused as a byte counter while loading, and `o`, `l` and
`r` are reused for many different things. `simple.c` names them `pc`, `sp`,
`fp`, `code_size` and uses `push()`/`pop()` helpers.

7. Opcodes packed into nibbles

Most instructions keep the operation in the top 4 bits of the byte
(`o=o/16`) and a small operand in the bottom 4 bits (`l=o%16`). A low nibble of
7 means "a 16 bit number follows", and 8 to 15 means "one more byte follows".
Bytes that the special opcode list catches first (like 12 = add) are handled
before this decoding, so the same number can mean two different things
depending on where it is checked. All opcodes are plain numbers with no names.

8. Function calls with an encoded operand

The call opcode's operand stores two things at once: the number of arguments
is `l/2` and whether to throw away the result is `l%2`. The function address
is found below the arguments on the stack. Addresses below 3 are the built in
functions (`0` is `getchar`, `1` and `2` are `putchar`). For other functions
the return address, old frame pointer and the operand are saved below the
stack pointer and 19 slots are reserved for the call frame.

9. Missing declarations and implicit `int`

There are no `#include` lines. `getchar` and `putchar` are used without being
declared. Old C compilers assume an undeclared function returns `int`, which
happens to work here.
