# 1995/vanschnitz

Difficulty: Hard

1. Meaningless macro names

Every macro name is a four letter mix of the letters `b`, `d`, `p` and `q`,
such as `pdbq`, `dpqb`, `dqbp` and `bqdp`. These letters are mirror images of
each other, so the names are very hard to tell apart when reading. For example
`dpqb` is the file's own name (`__FILE__`), `ppbd` is the format string
`"Move disk %d from peg %d to peg %d\n"`, and `pdbq` is the "already
initialized" flag. To understand the code you have to rename each one in your
head.

2. Junk tokens after preprocessor directives

Lines like `#endif   pqbd`, `#else    dpdp` and `main(){  pdbq` have extra
words after them. After `#endif` and `#else` the preprocessor ignores the extra
words (with only a warning). After `main(){` and `printf(` the word `pdbq` is a
macro defined as nothing, so it disappears. These words are just noise, there
to make the lines look like they mean something.

3. Program that includes itself (recursion with `#include`)

The file runs `#include dpqb`, which is `#include __FILE__`, so it includes
itself. The first time through, `pdbq` is not defined, so the setup part runs
and the `main()` function is written. Every later time, `pdbq` is defined so
only the `#else` part runs. Including itself again inside that part works like
a recursive function call, which is how the Towers of Hanoi recursion
(solve n-1 disks, move one disk, solve n-1 disks) is done.

4. Numbers stored as defined/undefined macros

The preprocessor has no variables that can change, so the number of disks is
stored as four bits. Each bit is a macro (`dqbp` = 1, `dbpq` = 2, `pbqd` = 4,
`bqdp` = 8) that is either defined (bit is 1) or not defined (bit is 0). The
`#if n&01` lines at the top convert the number `n` into these bits.

5. Binary counting with nested `#ifndef`/`#undef`

Before including itself, the file subtracts one from the bit macros using a
deep nest of `#ifndef ... #define ... #else #undef ... #endif`. This works like
subtracting one by hand in binary: zero bits are turned into ones until a one
bit is found and turned into a zero. After returning from the second include,
a similar nest adds one back so the caller sees its original value.

6. The program is computed at compile time

The program that is actually compiled is only one `printf` call. The
preprocessor writes out one copy of the format string for every move (joined
together because C glues neighbouring string literals into one string), and
then a second pass (when `qbdp` is defined) writes out the disk number and the
two peg numbers for every move as a long list of arguments, like `,1+1,1,3`.
So the puzzle is really solved by the compiler, and the program only prints
the answer.

7. Peg numbers from a state machine instead of a recursion

Peg numbers are never passed to the recursion. Instead the smallest disk
moves around the pegs in a fixed cycle 1 -> 2 -> 3 -> 1, tracked by the two
macros `pqdb` and `qdbp` (three states). Every other move is the only legal move
that does not use the smallest disk. Which way that disk goes depends on
where the smallest disk is and on whether the moving disk's number is odd or
even (bit `dqbp`). This is hard to recognize as the usual Hanoi algorithm.

8. Implicit declarations and implicit `int`

`main()` has no return type and `printf` is used without including
`<stdio.h>`. Old C allows both: an undeclared function is assumed to return
`int`, and so is a function with no return type. This makes the file contain
almost no normal C code.
