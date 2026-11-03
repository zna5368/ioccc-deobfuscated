# 1996/rcm

Difficulty: Hard

1. Macros hidden in the Makefile

The source never defines its macros. The Makefile passes them with `-D`, for
example `-DJ='if('`, `-DX='for('`, `-DU='B('`, `-DH='unsigned'`,
`-Dn='struct x'`, `-DR='malloc(sizeof(n))'`, `-DD='getchar()'` and
`-DN=putchar`. Reading `rcm.c` alone, a line like `J o & 4)` makes no sense
until you know `J` is `if(`. Even the headers are hidden: `#include T` and
`#include Y` become `<stdio.h>` and `<stdlib.h>` only through `-DT` and `-DY`.

2. Macros that contain half a statement

Several macros hold unbalanced pieces of syntax. `U` is `B(` so a call is
written `U 8, 1)` with only the closing parenthesis visible. `s` is
`else J O==`, which expands to `else if(O==`, so `s 6)` starts the next branch
of a long `if`/`else if` chain. `M` is `%=K`, so `(t M)` means
`(t %= 32768)`. `A` is `0,0,0,0,` and is used to pad the start of the extra
bit tables with zeros.

3. One function does everything

There is only one real function, `B(l, O)`. The second argument picks a job
and the first is its input: `O == 0` is the main program, `1` reads `l` bits,
`2` outputs a byte, `3` decodes a Huffman symbol, `4` frees a tree, `5` builds
a tree, `6` decodes a block, `7` copies a stored block, `8` sets up the fixed
tables and `9` reads the dynamic tables. `main` just calls `B(3, 0)`. In
`simple.c` each job is its own named function.

4. Decoy arguments

Many calls pass a first argument that is never used, such as `U 7, 8)`,
`U 5, 6)`, `U 2, 4)`, `U 9, 4)` and `U 6, (I = V, 3))`. The numbers look
important but only the second number (the job) matters. This hides which
calls actually take a value.

5. Global variables used as hidden parameters

Instead of passing a tree to the decode, build and free jobs, the code stores
it in the global `I` first: `I = V; U 2, 4);` frees tree `V`, and
`V = (U 288, 5), I);` builds a tree and then reads the result back out of `I`
with the comma operator. The function parameters `l` and `O` are also reused
as ordinary loop variables inside each job.

6. Redundant braces and semicolons

As the author admits, 184 useless `{`, `}` and `;` characters are scattered
around, for example `{ ; } ; { ; } ; { ; }`. They do nothing but make the
blocks look deeply nested and confuse the indentation.

7. Reversed array indexing

C defines `a[b]` as `*(a + b)`, so `b[a]` is the same thing. The code writes
`O[b]`, `C++[S]`, `t++[S]`, `O++[Z][b]++` (which is `b[Z[O++]]++`) and
`O[Z][G]` (which is `G[Z[O]]`). These are all normal array accesses.

8. Logic operators used as if statements

`o & 2 && U 16, 1);` only runs the call when the flag bit is set, and
`(t M) == (C M) && N (C++[S]);` only prints when the window is full. The
expression `O > 256 || (U O, 2), 0)` outputs a literal byte and then returns
0 so the length/distance code is skipped. These are `if` statements written as
expressions.

9. Loops with empty bodies

Almost all the work happens inside `for` headers. For example
`X O = 288; O--; Z[O] = ...);` fills a table in the increment part, and
`X; I->o; I = U 1, 1) ? I->O : I->o);` walks the Huffman tree one bit at a
time. Building a tree is a single `for` whose condition allocates missing
nodes and whose increment picks the left or right child.

10. Unsigned wrap around trick

After `X O = 17; O--; O[b] = 0);` ends, `O` has wrapped from 0 to the largest
unsigned value. The next loop starts with `O++`, which wraps it back to 0
without ever writing `O = 0`.

11. Comma operators inside the conditional operator

The code length decoder is one statement:
`f = o == 16 ? (o = l), 3 + U 2, 1) : o == 17 ? (o = 0), 3 + U 3, 1) : ...`.
Each branch of `?:` first changes `o` and then computes a repeat count, all
joined with commas. In `simple.c` this is an `if`/`else if` chain.

12. Recursion for tree freeing

Job 4 frees a tree with `i && (I = i->o, U 7, 4), I = i->O, free (i), U 1, 4));`,
a recursive call on the left child, then the right child, written as one
expression with the decoy arguments 7 and 1.

13. Single letter names

The tables `W`, `u`, `P`, `Q` and `L` are the deflate code length order,
length bases, length extra bits, distance bases and distance extra bits. `S`
is the 32 KB output window with read and write positions `C` and `t`, `j` and
`y` are the bit buffer and its bit count, and `Z`, `b` and `G` are the code
lengths, length counts and next codes used to build the canonical Huffman
trees.
