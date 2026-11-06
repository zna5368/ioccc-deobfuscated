# 2020/burton

Difficulty: Medium

1. Constants hidden in the Makefile

The code uses `B`, `I`, `T` and `S` but never defines them. They are passed
on the compiler command line (`-DB=6945503773712347754LL` and so on), so you
have to read the Makefile to know what the program does. Printed in hex the
decimal numbers make more sense: `B` is `0x606362e22242726a`. In simple.c they
are written as hex `#define`s with comments.

2. Constants encoded with XOR

The useful value `0x3131313131313131` (the text `"11111111"`, since `'1'` is
`0x31`) never appears. It is split into two random-looking numbers `B` and `I`,
and the code gets it back with `a=I^n`. XOR undoes itself: if `I = X ^ B` then
`I ^ B = X`. simple.c writes `ASCII_ONES` directly.

3. Constants derived by arithmetic on argc

The first parameter of `main` is the argument count, but it is named `b` and
used as a divisor. With one argument it equals 2, so `a/b` and `n/b` are
divisions by two that make the two other magic values: the mask
`0x0101010101010101` (the lowest bit of each byte) comes from
`(a/b&a)>>4`, and the multiplier `0x0100804020100804` comes from `a^n/b`. This
is also why extra arguments give "strange output": argc changes, so every
derived constant changes.

4. Multiplication used to move bits (no loops)

Converting a number to binary normally needs a loop over its bits.
Here `y` is multiplied by a constant that has one bit set in each byte.
Multiplying by such a number adds together several shifted copies of `y`, which
puts a different bit of `y` into the lowest bit of each byte. Masking with
`& r` keeps only those bits, and OR-ing with `a^r` (`0x30` = `'0'` in every
byte) turns each 0 or 1 into the character `'0'` or `'1'`. The one bit the
multiply can not place is added with the plain shift `y>>S`.

5. Integer used as a string

The eight characters are built inside one `long long` called `_`, and
`printf("%.8s\n",(char*)&_)` prints its memory as a string. The `%.8s` limits
printing to 8 bytes because there is no terminating zero. Which byte comes
first in memory depends on the machine (little or big endian), which is why
there are two sets of constants and why `prog_be` prints the bits reversed on
a little-endian machine.

6. Misleading identifier names

The variables are named `b`, `i`, `n`, `a`, `r`, `y`, `_` so that read in
order they spell "binary" (the README hint "v,a,r,i,a,b,l,e,s_NAME the
operation"), and `argc`/`argv` are renamed to `b` and `i`. The names say
nothing about what each value holds. simple.c uses names such as
`low_bit_mask`, `multiplier` and `ascii_zeros`.

7. Everything packed into one line

All the work is one declaration list on a single line, with several
operations nested inside each expression and relying on operator precedence
(`*` binds tighter than `|`, which is why `(a^n/b)*(y>>T)|y>>S` multiplies
first). simple.c splits each step into its own statement with parentheses.
