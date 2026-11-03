# 1989/paul

Difficulty: Hard

1. A program inside a string

The C code is only a Turing machine simulator. What it computes is decided by
the long string `m`, which is a Turing machine program made of three byte
instructions: current state, next state, and a byte of flag bits (`2` is the
symbol that must be under the head, `4` the symbol to write, `8` move right,
`16` print). Reading the C code tells you nothing about Fibonacci numbers; you
have to decode the string by hand. `simple.c` names the flag bits
`READ_SYMBOL`, `WRITE_SYMBOL`, `MOVE_RIGHT` and `PRINT_NEXT`.

2. Instructions chosen to look like words

The low bit of every state is thrown away with `~1&`, so `B` and `C`, or `n` and
`o`, are the same state. This let the author pick letters that make the program
look like text ("beleY", "ijainnjmam") instead of numbers. The flag bytes are
also picked from printable characters such as `,`, `%` and `&`, whose unused
high bits are ignored. `simple.c` hides the `~1&` inside a `STATE()` macro.

3. Error messages that are also code

The usage and error messages are stored at the front of the same string as the
program, separated by `\0` bytes. Instructions are searched three bytes at a
time, and the message lengths are chosen so that these steps jump over the
`\0` bytes and land exactly on the `j` of `jt,`. Some triples in the messages
are even real instructions: `tri` in `[string]` and ` bl` in `Usage: black`
are used by the Fibonacci program. The messages are found with magic offsets,
`23+m` and `39+m`. `simple.c` keeps the string in one piece (moving it would
break the program) and gives the offsets the names `NO_MEMORY_MESSAGE` and
`ALIGNMENT_MESSAGE`.

4. Overlapping print character

When the print bit is set, the character printed is `u[3]`, which is the first
byte (the state) of the *next* instruction in the string. For example `BVP`
is followed by `11/`, and `C0R` is followed by `\n\n'`, so the `1` and the
newline that get printed are also the states of other instructions. The output
characters are hidden among the states.

5. XOR linked list

The tape is a doubly linked list, but each cell holds only one value: the
address of its left neighbour XOR the address of its right neighbour. If you
know where you came from, XOR with that address gives you where you are going.
The macro `z(a,b)` does this. `simple.c` calls it `other_neighbour()`.

6. Data hidden in the low bit of a pointer

Memory from `malloc` always starts at an even address, so the lowest bit of the
XOR of two addresses is always 0. The program stores the tape symbol (0 or 1)
in that bit with `*q|=1` and `*q&=~1`, and the `~1&` in `z` removes it before the
value is used as an address. The `k` macro even checks that `malloc` returned
an even address, and prints "Unusable memory alignment" if not.

7. Remembering which way you came

To use the XOR list, the program keeps `p` (the cell it just left) and `b` (the
side `p` is on). If the head moves back toward `b`, the next cell is simply
`p`; otherwise it XORs to find the other neighbour and allocates a new cell if
there is none. `b` starts as `3` so that it never matches a direction on the
first move. `simple.c` names these `previous`, `previous_side`, `LEFT`, `RIGHT`
and `NOWHERE`.

8. Macros that hide statements

`v` is `(void)printf(`, `x` is `),exit(1);`, so `v m x` is a whole "print the
message and quit" statement. `k(l)` allocates a cell, checks it twice, and ends
with `*l=` so the code after it, like `k(q)0;`, finishes an assignment. `y` is
the closing `)` of a `for` plus the whole loop body, so the two loops in `r`
look like they have no body at all. `simple.c` replaces these with the
`new_cell()` function and ordinary loops.

9. Single letter names and reused variables

`f` is `long`, `a` is first the argument count and then the machine state, `c`
is the argument list in `main` but a scratch pointer in `r`, and `n` is the
start of the program. `p` and `q` even swap roles between `r` and `main`.
`simple.c` uses separate, named variables such as `state`, `start`, `head`
and `instruction`.

10. Hidden trace routine

The function `r` prints the tape around the head and sleeps for a second, but
the calls to it are commented out, so it looks like dead code with no purpose.
It walks the XOR list in both directions starting from the head. `simple.c`
names it `trace()` and keeps the calls commented out in the same places.

11. Old style C

`main` and `r` use K&R parameter lists, `r` and `main` have no return type,
`malloc` is declared by hand as `char *malloc()` and `exit` and `sleep` are used
without being declared. `simple.c` uses modern function definitions.
