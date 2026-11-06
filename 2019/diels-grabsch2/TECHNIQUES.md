# 2019/diels-grabsch2

Difficulty: Hard

1. Precomputed hash state (fake self-hash)

The program appears to hash its own source, but it never reads `prog.c`. The
first 1280 bytes of the file (exactly 10 SHA-512 blocks, ending at the `"` that
opens the string) were hashed ahead of time by an outside script. The 8 internal
hash values at that point are stored in the first 104 characters of the string
and loaded with `f(8,a[e]=d(e))`. The program only has to finish the last block
and the padding block itself.

2. Data hidden inside the hashed text

The string literal plays two roles at once. Its 125 characters are the raw text
of the final 128-byte block (`o(e)` reads them 8 bytes at a time as big-endian
numbers), and the same characters are also decoded as numbers. The 3 bytes that
come after the string in the file, `";` and a newline, cannot appear inside the
string, so the last 8 bytes of the block are instead stored as a number encoded
in characters 104..116 and read with `d(8)` into `c[15]`.

3. Custom base-31 text encoding

`d(b)` turns 13 characters into a 64-bit number using base 31, where each digit
is the character minus 96 (`` ` `` is 0 and `~` is 30). Only lowercase letters
and a few symbols are used, so the encoded numbers look like random text and
need no escape characters.

4. Round constants computed instead of written

SHA-512 needs 80 well known 64-bit constants that would be easy to search for.
They are the fractional parts of the cube roots of the first 80 primes, so the
program computes them at runtime: `v` tests for a prime, `l` finds the n-th
prime recursively, and `p`/`q` find the cube root by bisection (try each bit
from high to low and keep it if the cube is still below the prime).

5. Hand-made extended precision arithmetic

Standard C has no 128-bit integers, so `t` cubes a number split into three
24-bit pieces, multiplying at most two pieces at a time and throwing away bits
with right shifts so nothing overflows. Some small terms are left out entirely.
The shift amounts were found by trial and error and only work for these 80
primes, which makes the formula very hard to recognise.

6. Function generating macros

The macros `i`, `h`, `j` and `g` each expand into a whole function definition
(one argument, three arguments, three pointers, or a loop that accumulates a
value). `f(n,body)` expands into a counting `for` loop with a hidden variable
`e`. Reading the code means mentally expanding every macro first.

7. Meaningless single letter names

Every function and variable has a one or two letter name (`_`, `_a`, `u`, `r`
for the SHA-512 sigma functions, `s` for choose, `x` for majority, `n` for a
round, `k` for compression). Parameter names are reused for different things,
and the typedefs `y` and `z` hide `unsigned char` and `unsigned long long`.
Even the rotate function `b` shares its name with a parameter.

8. Conditional expressions instead of if statements

The state rotation in `n` is a loop `a[7-e]=e-3?e-7?a[6-e]:...:d+a[3]` that
uses nested `?:` on the loop counter to pick which of the 8 words gets the new
values. `e?e-15?0:11264:1ull<<63` builds the padding block the same way, and
`a|c*m(...)` sets a bit only when `m` returns 1 instead of using `if`.

9. Rolling message schedule

Instead of a normal 80 word message schedule array, only 16 words are kept.
`f(16*(*b%16>14),...)` runs a loop zero times or 16 times depending on whether
the round number ends a group of 16, then rewrites all 16 words in place.

10. Out of order declarations

The data string `w` is declared `extern` at the top but only defined at the
very end of the file, so that the string falls into the final hash block. The
`main` return value `!w` is just a disguised `0`.
