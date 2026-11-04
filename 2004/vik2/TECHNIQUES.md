# 2004/vik2

Difficulty: Hard

1. Preprocessor directives hidden behind one letter macros

In `vik2.orig.c` the first lines say `#define O #ifdef`, `#define Q #undef`,
`#define C #define`, `#define G #else`, `#define D #ifndef`, `#define I #endif`
and `#define l #include FNAME`. The rest of the file is written with these
letters, so `O a` is really `#ifdef a`. A normal preprocessor does not turn a
macro back into a directive, so the file has to be run through `cpp` once to
produce the real directives and then compiled. The `vik2.c` and `vik2_1.c`
files in the repository are this already expanded form.

2. Numbers stored as defined or undefined symbols

The program never stores a value in a macro. Each bit of a number is its own
symbol: if the symbol is `#define`d the bit is 1 and if it is `#undef`ined the
bit is 0. The ALU register `X` is the eleven symbols `c d e f g h Z j k m n`,
register `Y` is `t s u v w x y z A B r`, the carry is `q` and the "equal" and
"less than" flags are `J` and `_q`. The memory variables are made the same way,
for example the number being tested is `_h _t _3 _4 _5 _6 _7 _8 _9 _r` with an
always set lowest bit. Because the names are single letters picked in no order,
there is nothing to tell the reader which symbols belong together.

3. Logic made only from `#ifdef` and `#else`

Adding is done with a one bit full adder written as nested `#ifdef a` /
`#ifdef b` / `#ifndef q` blocks that `#define` or `#undef` the sum and carry
symbols for every combination of inputs. The 11-bit `ADD` copies one bit of `X`
into `a` and one bit of `Y` into `b`, runs the adder and copies the sum back,
eleven times in a row. `simple.c` keeps this as the `add_bit` function and a
loop in `add`.

4. Including itself as a function call

`#include __FILE__` (the `l` macro) makes the file read itself again. Before
each include a flag symbol is defined that picks which part of the file runs:
`o` is `CLR_X`, `E` is `CLR_Y`, `_L` is the one bit adder, `F` is `ADD`, `H` is
`INC2`, `_u` is `SUB`, `_w` is `TST` and `_x` is one step of the prime search.
The included copy runs the matching `#ifdef` block, clears the flag and returns,
just like calling a function. In `simple.c` each of these is a C function.

5. Loops made from repeated and recursive includes

There is no loop statement in the preprocessor. The file includes itself ten
times with `_K` set, each of those includes itself ten times with `_J` set and
each of those ten times with `_x` set, so one round runs 1000 steps of the
search. After the round, `#ifndef K` includes the file again to start another
round until the stop symbol `K` is defined. This is replaced by a `while` loop.

6. Subtracting and comparing with only an adder

`SUB` flips every bit of `Y`, adds it to `X` and then adds 1, which is the two's
complement way of computing `X - Y`. The carries of the two additions are joined
by toggling `q`. `TST` is a `SUB` that then checks whether every bit of `X` is
undefined (equal) and whether the carry is missing (less than). This destroys
`X`, so the values have to be loaded again after every comparison.

7. Hidden prime test using sums of odd numbers

The sum of the first `k` odd numbers is `k*k`. The program keeps a running
sum, adds the next odd number when the sum is smaller than the candidate and
subtracts the next odd number when it is larger. When the sum equals the
candidate it has found `a*a - b*b = (a - b) * (a + b)`. The first match has the
two factors closest together, so the candidate is prime only if the last added
odd number is exactly 2 more than the last subtracted one. The odd number
counters start with all bits set, which is -1 in 11 bits, so the first `+ 2`
gives 1. Nothing in the code hints at this method.

8. Printing a number by writing out its bits

To print a prime the program undefines `i` and includes itself, which emits the
text `printf("%d\n", 1`. Back in the caller, an `#ifdef` for each bit emits
`| 2`, `| 4`, `| 8` and so on, and finally `);`. The compiler only ever sees a
`printf` whose argument is an OR of powers of two, such as `1 | 2 | 16`. The
first prime is printed as `1 + 1`.

9. One file playing three roles

The same file is the program text, the ALU and the prime search. `#ifdef i`
and `#ifdef _N` decide whether this pass writes the `int main() {` header, the
start of a `printf` call or does the computation. In `vik2.c` the main function
includes `vik2_1.c`, a copy of the same file that holds the computation.

10. Lines broken every three characters

The small amount of real C in `vik2.orig.c` is split with backslash-newline
after every three characters, as in `pri\ ntf(` and `e<s\ tdi\ o.h>`. The
preprocessor joins these lines back together, but a reader has to do it in
their head.

11. Stop limit passed in from the command line

The number of primes is set by `-DSTOP=_5` on the compile line. In the
original, `STOP` is placed after `#ifdef` so the first `cpp` pass turns it into
`#ifdef _5`, the symbol for the bit worth 32, and the search stops when the
candidate reaches 32. In the repository's `vik2_1.c` this check is written
directly as `#ifdef _5`. `simple.c` uses `LIMIT_BIT`.
