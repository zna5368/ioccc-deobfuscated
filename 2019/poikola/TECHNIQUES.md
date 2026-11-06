# 2019/poikola

Difficulty: Insane

1. Compile date chooses the program

`R` is `__DATE__`, the date the compiler ran, for example `"Nov  6 2026"`. The
macro `D` turns characters 7 to 10 into the year, and `k` turns the month name
into a number by looking at its third letter (`q'p'?9:` means "if the third
letter is `p` the month is 9"). The program puts the year, month and day into
Mike Keith's day of week formula and takes the result modulo 3 instead of 7. The
answer picks which label to jump to: `vuokatti` (Fibonacci numbers),
`virpiniemi` (SHA-3 digest) or `ounasvaara` (primes). Because of this the same
source shows something different each day it is compiled, and someone who
compiles it once only sees one of the three programs.

2. Computed goto with a table of labels

`const void *j[] = {&&laajavuori, &&ruka, ...}` is a GNU C extension: `&&label`
gives the address of a label and `goto *j[Q]` jumps to whatever label is stored
at index `Q`. The index is hidden in expressions like `j[!!b]`, `j[!0]` and
`j[-~(P-P)]` (all equal to 1, the Keccak permutation `ruka`). The Keccak
permutation is reused like a function without being one: before jumping to it
the code stores the "return address" in `Q` (3 for `lahti`, 0 for
`laajavuori`), and `ruka` ends with `goto *j[Q]`. The simplified version makes
these real functions and uses a `switch` for the three modes.

3. Jumping into the middle of loops and `if` blocks

The labels `puijo` and `lahti` sit inside an `if` inside a `for` loop. After the
permutation the program jumps straight back into the middle of that block to
reset the lane counter `F`. This makes the flow very hard to follow because a
block can be entered without passing its condition. There is also a label `C`
for the sieve and `$` (a dollar sign is allowed in names by GCC) for the
padding step.

4. Single letter names and recycled variables

Every variable is one letter (`T` is the digest size, `Y` and `H` the file size,
`F` the current lane, `I` is a loop counter used by the sieve, the permutation
round counter and the output loops). `P` starts as `argc`, becomes `2` after
`U[--P]`, is used as the constant 2 in many places (`Y/P`, `I*P`, `v[(G P)%5]`)
and is finally reused to hold a hex digit. `_` and `d` are the Keccak constants
`1<<63` and `1<<31` at first and then become the two Fibonacci numbers.

5. Constants built from other values

Small numbers are written in strange ways: `M=-~9` is 10, `r++` makes `r` 1,
`b` is the backspace character `'\b'` (8), `'?'` is 63, `0100` is octal 64,
`0620` is octal 400, `0x64` is 100, `037` is 31, `o` is 25, `~-P` is `P-1`,
`!o` is 0 and the Fibonacci loop stops at `'^'` (94). The table entry
`N[1]=P` only equals the Keccak rotation 3 because `argc` is 3, and `O[17]=P`
is 2 because `P` was decremented just before. `4[N]` and `11[O]` use the fact
that `a[i]` and `i[a]` mean the same thing in C.

6. Round constants hidden as bit expressions and reversed

The Keccak round constants in `w[]` are written as OR combinations of
`_` (bit 63), `d` (bit 31) and small decimal numbers, for example
`_|d|32776` is `0x8000000080008008`. They are also stored in reverse order
because the round loop counts down (`for(I=b*3; I--;)`) and uses `c.K[!1]^=w[I]`.
The extra last entry `w[24]` is reused as the `0x80` padding bit. The simplified
version lists the 24 constants in hexadecimal in the normal order.

7. Macros that hide code

`g` is a whole `for(a=0;a<5;a++)` loop header, `G` is `a+`, `Z` reads one
leftover byte into the padding word, `L` reads one byte of a 64-bit word and
`q` is `__DATE__[2]==`. Reading the code requires expanding these in your head;
the month macro `k` is a long chain of nested `?:` operators.

8. Undefined behaviour that the output depends on

Several expressions change and read a variable without a sequence point, so the
result depends on the compiler. `L|L|L|L|L|L|L|L` uses `l[++S] << b*S` eight
times: Clang reads a correct little endian word, but newer GCC combines all the
bytes with OR and shifts by 56, so the "SHA-3" digest is only correct with some
compilers (the README marks this as a feature that should not be fixed). The
simplified code keeps this expression on purpose, behind the macro `NEXT_BYTE`,
so it prints the same digest as the original with each compiler. The sieve setup
`x[I++>>4]|=(1<<(I&4[N]))` has the same problem, which only matters for files of
fewer than 4 bytes.

9. Prime sieve with a hidden bug that removes 2

The sieve stores 16 bits in each `unsigned int` of `x` (`x[I>>4]` and
`1<<(I&15)`). The first pass starts with `y=0` instead of `y=2*2`, so it
crosses out 2 itself together with its multiples. That is why the "oddest
prime" 2 is missing from the output. The simplified version just starts the
candidates at 3.

10. Hand written file reading and number parsing

The program does not use `atoi` or `fread`: it parses the bit count digit by
digit (`T=T*M+U[!u][a]-W`), divides by 8 with `T>>=P` (`P` is `argc`, 3), opens
the file with flags `F` (0, read only) and maps it with `mmap(NULL,H,r,r,X,u)`
where `r` (1) means both `PROT_READ` and `MAP_SHARED` and `u` (0) is the offset.

11. Dead code

`z = b & 7` is always 0, so the two blocks `if(H<z)` and `if(z)` (and the jump
to `puijo` inside them) never run. They only add more labels and `goto`s to
think about.

12. Misleading layout and comments

The source is shaped as a picture to honour the ski jumper Matti Nykanen, and
the labels are Finnish ski jumping hills. `http://www.ioccc.org/` looks like a
URL but is the label `http:` followed by a `//` comment. Comments like
`/*TODO: FIX THIS */`, `/*foob*/` and the commented out
`/*m= o/i; v[a]=a+n;return 0;*/` at the end are only noise.
