# 2024/carlini

Difficulty: terrifying

1. Shorthand macros

`#define Z while(`, `#define A *Q++`, `#define W for(i=0;i<K;i++)` and
`#define P 1<<24` make the code very short and hard to read. `W` is
especially tricky: it loops over whichever `i` and `K` are visible where it is
used, so in `main()` it uses the local `i`/`K` but inside `d()`, `h()` and
`w()` it uses the globals. `A` reads the next value from whatever `Q` happens
to be in scope (a `char *` string, an `int *` stream, or the trap port).
`P` has no parentheses, which only works because it is never used next to
another operator.

2. Typedef hiding `int`

`typedef int _;` makes every declaration look like `_ i,q[P],...` or
`_*w(_*Q)`, so it is not obvious that they are plain `int`s.

3. Single-letter names and reused variables

Almost every variable is one letter, and many are reused for unrelated jobs.
`B` is first the bit history of the arithmetic decoder and later the gate
table. `R` is first a copy of the circuit program for sub-circuit calls and
later holds the ROM bytes and micro-op columns. `p` is the arithmetic
decoder's position, the bit-plane loop counter in `d()` and the old gate value
in the main loop. The globals `K` and `I` are general scratch values used by
every function. In `simple.c` each job has its own named variable.

4. Shadowed names

There are four different `Q`s: the global `char *Q` (compressed string), the
`int *Q` parameter of `w()`, the local `int *Q` in `main()`, and a `char *Q`
inside a block in the main loop. `main()` also declares its own `K`, `i` and
`g` that hide globals with the same names, so the same letter means
different variables depending on which function you are in.

5. Data hidden in a string with an unusual number base

The whole circuit is stored in the string `Q`. It is read as a base 1968
number by an arithmetic decoder (`s()`): the first character gives a digit
0..122 (after skipping `\n`, `\r`, `"` and `\`), and a second part worth up
to 16 is made only of whitespace or `{`, `}`, `;` followed by whitespace,
which the IOCCC size counter does not count. The odd expressions
`(K^8)*2%5` and `(K^6)%3*4` turn those characters into small numbers.

6. Arithmetic coding and LZ compression

`s()` is an adaptive binary arithmetic decoder (each "context" counts how
many 0s and 1s it has seen), `h()` reads variable length numbers from it,
and the first loop of `main()` is an LZ decompressor that either copies an
earlier run of numbers or writes a new signed literal. None of this is
labelled, so the reader must recognise the algorithms from the arithmetic.

7. The circuit is itself a tiny compressed program

`w()` expands the decompressed numbers into about 158,000 logic gates. Gate
inputs are stored relative to the current gate (`a-I`), numbers above
`2e5` refer to sub-circuit arguments, opcode 6/8 calls a sub-circuit and
opcode 7 repeats the previous instruction. The repeat works by
self-modifying code: it decrements its own counter in place (`--A`), adds
constants to the fields of the previous instruction, and jumps back
(`Q=f`). Because of this, every call first copies the entire program into
`R` (`K=P; W R[i]=q[i];`) so the original stays untouched.

8. Nested ternaries instead of a switch

The gate evaluation
`T[M]=r<2?i^r:r<4?r^3?i|K:i&K:r^5?i^K:i?K:I;` is a hidden switch on the gate
type (wire, not, or, and, xor, mux). `simple.c` uses a `switch` with an
`enum` of gate names.

9. Bit tricks in place of normal operators

`-~x` is `x+1`, `N^7` means `N != 7`, `r^3` means `r != 3`, `6*-~i` is
`6*(i+1)` and `32+F*13` chooses between `' '` (32) and `'-'` (45). These are
shorter but hide the meaning.

10. Magic numbers

`fcntl(0,4,O_NONBLOCK)` uses `4` instead of `F_SETFL`, `malloc(7e4)` and
`2e5` are floating point constants used as integers, `325` is where the
keyboard table starts in the character table, `21` is the spacing of the
print columns, `128` is the keyboard rate and `exit(42)` is the missing
argument error. `simple.c` gives these names.

11. Inverted, double-buffered, hierarchical bitmask

To run fast the program only recomputes gates whose inputs changed. `f`
holds two copies of a three-level bitmask (blocks of 4096, 64 and 1 gates)
where a **set** bit means "skip" and a cleared bit means "needs work", the
opposite of what one would expect. `y^S*(g<=M)` chooses which copy to clear:
gates that come later are updated in this sweep, earlier ones in the next
sweep. The skip loop leaves the level loop by setting its counter to zero
(`K=0`) instead of using `break`.

12. Type punning a print table

`*(_*)(T+n+...*21)%18` reads four 1-byte gate values as one little-endian
`int` and takes it mod 18. The drum character table is stored in a permuted
order (with some duplicates) so that this strange number lands on the right
character. `simple.c` builds the int from the four bytes explicitly.

13. Reading with side effects inside expressions

Values are pulled off streams with `A` (`*Q++`) inside larger expressions,
for example `if(K&I&A)` uses `&` instead of `&&` so the pointer always moves
forward. Where the keyboard bits are written depends on how far `Q` has
moved, which turns out to always be 5 gates after the trap flag. In
`simple.c` the trap port fields are named with `#define`s.
