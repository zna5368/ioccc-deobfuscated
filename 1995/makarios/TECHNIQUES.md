# 1995/makarios

Difficulty: Medium

1. Whole program in one tiny function

In `makarios.orig.c` the entire program is `main(n,i,a,m)` with four untyped
parameters. Modern compilers reject that `main`, so `makarios.c` moves the body
into a function `pain` (a pun on "main") and `main` only calls
`pain(n,i,a,0)`. Either way the palindrome test, the prime test, the printing,
and the outer counting loop are all squeezed into two lines with no names.
`simple.c` splits the tests into the named helper functions
`is_octal_palindrome()` and `is_prime()`.

2. Misused `main` parameters as variables

`main(n,i,a)` reuses `argc` (`n`) as the number being tested and the `argv`
slots (`i`, `a`) as scratch integers. Because the program is run with no
arguments, `argc` is 1, so the first `++n` makes the count start at 2. In
`simple.c` the counter starts at `argc + 1` in a normal `for` loop.

3. Old-style implicit `int` declarations

`pain(n,i,a,p)` has no types at all. In old C, an untyped parameter or
function is assumed to be `int`, so the code relies on `i`, `a`, and `p` all
being plain integers even though `main` declares two of them as `char **`.
`simple.c` gives every variable an explicit `int` type.

4. Whole algorithm packed into a `for` condition

`for(a=0; cond ? x : y;);` has an empty body. All the work happens inside the
condition using the `?:` and comma operators. While `a<i` is true, the comma
sequence `a=a*8+i%8, i/=8, p=..., 1` moves one digit and returns 1 to keep
looping. Once `a>=i`, the other branch of `?:` runs the prime test. `simple.c`
writes the two phases as separate `do`/`while` and `for` loops.

5. Palindrome test by half reversal

Instead of converting the number to a string, the code peels octal digits off
the bottom of `i` (`i%8`, `i/=8`) and pushes them onto `a` (`a*8+...`) until
`a` is at least as large as `i`. The number is a palindrome when the two halves
match: `a==i` for an even number of digits, or `a/8==i` when the middle digit
should be ignored. The result is stored with a bitwise `|` instead of `||`.

6. One variable doing two jobs

`p` first holds the palindrome flag (0 or 1). The prime test then does `++p`,
so a non-palindrome gets `p==1` and `n%1==0` ends the loop immediately, while a
palindrome gets `p==2` and starts trial division. `p` then counts up as the
divisor. This hides the "is it a palindrome?" check inside the divisor
counter. `simple.c` uses a separate `palindrome` flag and `divisor` variable.

7. Short-circuit operators as `if` statements

`(n-++p || printf("%o\n",n)) && n%p` means: if the divisor has not reached
`n`, check whether it divides `n`; if it has reached `n` (no divisor found),
print `n` in octal. `printf` returns a nonzero count, and `n%n` is 0, so the
loop then ends. A divisor that divides `n` also makes the expression 0 and
ends the loop. `simple.c` uses an ordinary `if` with `is_prime()`.

8. Loop that ends on integer overflow

`while(i=++n)` only stops when `n` wraps around to 0, which is both undefined
behavior and practically never reached because trial division gets very slow.
It also copies `n` into `i` in the same expression. `simple.c` keeps the same
`number != 0` stop condition but writes it as a plain `for` loop.

9. Missing headers

The source has no `#include` lines at all and relies on `printf` being
implicitly declared. `simple.c` keeps this so it uses the same headers as the
original.
