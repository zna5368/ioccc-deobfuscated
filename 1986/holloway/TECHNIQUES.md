# 1986/holloway

Difficulty: Hard

1. Numbers built from one macro

Only `#define e 3` holds a real number. Every other constant is built from it:
`g` is `e/e` = 1, `h` is `(g+e)/2` = 2, `f` is `e-g-h` = 0, `j` is `e*e-g` = 8
and `k` is `j-h` = 6. The code then writes `f` for 0, `g` for 1 and so on, so
even simple values such as `for(a=f;a<j;++a)` (which is `for(a=0;a<8;++a)`)
must be worked out by hand. simple.c uses plain numbers.

2. Recursive `main` used as several functions

`main` is the only function. Its first argument `m1` chooses what it does:
with 1 (no command line arguments) it is the real program, with 2, 3 or 4 it
computes one octal digit, and with larger values it builds the message. The
`switch(m1-=h)` subtracts 2 first, so the `case` labels `f`, `g` and `h`
(0, 1, 2) really mean calls with 2, 3 and 4. simple.c splits this into
`high_digit`, `middle_digit`, `low_digit` and a normal `main`.

3. Arguments passed with the wrong types

The second argument of `main` is declared `char **s`, but the program passes a
`char` buffer and also plain integers cast to `(char *)`. Inside it uses
`char *t = s` for the buffer and `n=(int)s` for the number, so one parameter
is used as both a pointer and an integer depending on the call.

4. Recursion that fills a string

The `default:` case writes one character at `*t` and calls `main` again with
`t+g` (the next byte) and a smaller `m1`, until `m1` reaches 0 and it writes
the terminating `'\0'`. This is a hidden loop over the 14 characters of the
message. Each character gets a key `n` (13 down to 0) from the value of `m1`.
simple.c uses a `for` loop.

5. Characters built from octal digits

`*t=(*t<<e)|main(h+a++,(char *)m1)` shifts the character left by 3 bits and
adds a new digit three times, so each character is three octal digits (for
example `'h'` is `0150`). The string `"hello world!\r\n"` never appears in the
source; every digit is computed by a different formula.

6. Bit masks for the high digit

`case f:` sets `d=1`, `c=2`, `b=4`, `a=8` with a chain of shifts, then returns
`m(n,a|c)|m(n,b)|m(n,a|d)|m(n,c|d)`. The macro `m(n,a)` checks that all bits
of `a` are set in `n`. Together they give 1 for the keys of letters and 0 for
the keys of `' '`, `'!'`, `'\r'` and `'\n'`, which is the high octal digit.

7. Fibonacci-like sequence for the middle digit

`case g:` fills `o[]` with `o[b]=o[b-h]+o[b-g]+c` (each number is the sum of
the two before it plus a constant), using different start values and
constants (`'D'` or `'\r'-'\b'` = 5) depending on the key. The answer is
`o[b-g]%n+k-h`. The sequence has nothing to do with text; the start values
were simply picked so the results are the right digits.

8. Prime factors hide the low digit

`tab2[]` holds twice the first 14 primes, and `l(x)` divides by `h` (2) to get
the prime back. `tab1[]` holds products of primes, such as 88319 = 7 * 11 *
31 * 37. `case h:` returns the index of the first product that the key's
prime divides, which is the low octal digit. The zeros in `tab1[]` are unused
slots so the indexes come out right.

9. Switch fall through

`case h:` has no `break` or `return` after its loop, so if no product matches it
falls into `case g:` and uses the middle digit formula instead. This never
happens for this message, but it makes the control flow harder to follow.

10. Variable shadowing

`main` declares `int b`, and inside `if(m1==1)` a new `char b[...]` hides it.
The same name is used for an integer counter and for the message buffer.
