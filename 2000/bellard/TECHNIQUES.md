# 2000/bellard

Difficulty: Hard

1. Single-letter global variables

Every variable (`m`, `N`, `t`, `a`, `p`, `i`, `e`, `j`, `s`, `b`, `c`, `U`) is a
one-letter global declared on the first line, and many are reused for unrelated
jobs. For example `a` is the swap temporary in `f()` and the carry in `main()`,
and `s` is both the butterfly half-width and the bit mask in the bit-reversal
loop. Because they are global, `f()` silently changes values that `main()` also
uses. In simple.c they are local variables with names like `half`, `carry` and
`inverse_size`.

2. Unexplained magic numbers

`754974721`, `362`, `415027540`, `6972593` and `1 << 23` appear with no
explanation. `754974721` is the prime `45 * 2^24 + 1`, chosen so that the
arithmetic "mod m" has roots of unity needed for an FFT. `362` is such a root
(it returns to 1 after being multiplied by itself `2^24` times) and `415027540`
is its inverse, used to undo the transform. `6972593` is the exponent of the
prime `2^6972593 - 1`. simple.c gives each one a named `#define`.

3. Hidden mathematical algorithm (Number Theoretic Transform)

The function `f(d)` is a Fast Fourier Transform performed with whole numbers
modulo a prime instead of with floating point. Multiplying two big numbers is
the same as multiplying two polynomials whose coefficients are the digits, and
an FFT does that quickly: transform, multiply point by point, transform back.
Nothing in the code names any of this, so a reader has to recognise the
butterfly `b = *p + p[s], p[s] = (*p - p[s]) * c` pattern to know what it is.

4. Twiddle factor computed by repeated squaring in the loop header

`for (s = 1 << 23; s; s /= 2, d = d * 1LL * d % m)` squares the root every time
the block size halves, even for levels that are skipped by `if (s < N)`. This
means the correct root for each level appears without ever being computed
directly. simple.c keeps this but explains it and uses `continue` for the
skipped levels.

5. Comma operators packing several statements into one

The inner butterfly loop body is four assignments joined by commas, and the
carry loop `a += ..., *p++ = a % 10, a /= 10` is three statements in one. This
avoids braces and hides the order of operations. simple.c writes each step on
its own line.

6. Pointer walking instead of indexes

`p` is moved through the array with `*p++` and `p += s`, while `p[s]` reaches
ahead to the partner element. Reading the code requires keeping track of where
`p` points at every moment. At the end `while (!*--p);` leaves `p` on the top
digit and the printing loop walks it backwards. simple.c uses plain array
indexes such as `digits[i]` and `digits[i + half]`.

7. Bit tricks for the bit-reversal permutation

`for (s = N / 2; !((j ^= s) & s); s /= 2);` adds 1 to `j` as if its bits were
written backwards (it flips bits starting from the top until it sets one that
was clear). Combined with `if (++i < j)` swapping, this reorders the FFT output.
The loop has an empty body and also depends on `i` being left at 0 by the
previous loop.

8. Loop counter that is really the exponent bit mask

`e` starts at `1 << 22` and `while (e /= 2)` runs 22 times, while
`6972593 & e ? 2 : 1` tests the current bit of the exponent. This is binary
exponentiation (square, then double if the bit is 1) disguised as a loop over a
shrinking power of two. The starting value `*t = 2` silently accounts for the
top bit `2^22`.

9. Modular inverse built up incrementally

`U = U * 1LL * (m + 1) / 2 % m` multiplies by `(m + 1) / 2`, which is the
inverse of 2 modulo `m`. Since `N` doubles each round, `U` is always `1/N`,
the scaling needed after the inverse transform. The purpose of this line is
not obvious without knowing modular arithmetic.

10. Tricky expression spacing and undefined behaviour

`*p++ = (*p * 1LL ** p % m) * U % m;` writes `** p` so it looks like a double
pointer dereference, but it is really `1LL * *p`. The same statement also reads
`*p` and increments `p` without a sequence point, so the result depends on the
compiler evaluating the right side first. simple.c writes
`digits[i] = mulmod(mulmod(digits[i], digits[i]), inverse_size);`.

11. Implicit int and missing declarations

`f (d)` and `main ()` have no return types or parameter types, and `printf` is
used without including `<stdio.h>`, relying on old C rules where everything
defaults to `int`.

12. Final subtraction hidden at the end

The number is computed as `2^6972593`, and only just before printing does
`t[0]--` subtract one. This works because a power of two never ends in the
digit 0, so no borrow is needed.
