# 2025/ncw2

Difficulty: Terrifying

1. Misleading comments and names

The comments `/* C64 EMULATOR */`, `/* ROM */` and `/* CPU */` make the
reader expect a 6502 CPU and memory.  There is no emulator.  The array `p`
is not a ROM image and the loop is not a CPU; together they are a FRACTRAN
interpreter running a FRACTRAN program.  simple.c keeps only the title
comment and uses names like `boot_screen` and `pi_bit()`.

2. The whole program written in FRACTRAN

The real program is the list of fractions in `p[]`, stored as pairs
`numerator,denominator` (`1,2`, `1,3`, ... `5423170,61`, ...).  FRACTRAN
has one rule: find the first fraction `f` where `n * f` is a whole number,
replace `n` with `n * f`, and start again from the top of the list.  The
state of the program (which line it is on, loop counters, the character to
print) is stored as the powers of prime factors inside the one number `n`,
which starts at `59`.  Large primes such as 61, 73, 79 ... act like line
labels and small primes act like variables.  To see what it does you must
factor the numbers and follow the state machine by hand, which is why
simple.c skips the interpreter and writes the result directly.

3. Strings hidden as prime factors

No text appears in the source.  Each output byte is spelled out by which of
the primes 3, 5, 7, 11, 13, 17, 19 and 23 divide `n`: divisible by 3 means
bit 0 is set, by 5 means bit 1 is set, and so on up to 23 for bit 7.  An
extra factor of 2 means "a byte is ready to print".  The first fractions
`1/2`, `1/3`, `1/5` ... `1/23` then remove those factors one at a time to
clear the byte.  simple.c writes the decoded colour escape codes, the
`**** COMMODORE 64 BASIC V2 ****` banner, `READY.`, the `10 PRINT` line and
`RUN` as an ordinary string.

4. Random looking maze from the digits of pi

The maze looks random but is the binary digits of pi (`11.0010010000...`),
`╱` for 1 and `╲` for 0, and it repeats after 300 characters so that `n`
fits in 64 bits.  These digits are built into the fractions and only appear
when the program runs.  simple.c stores the 300 bits as the hex string
`c90fdaa2...` and loops over it.

5. Prime numbers hidden in a string of escape codes

`g("\3\5\a\v\r\21\23\27")` looks like a string of control characters.  It
is really the list of primes 3, 5, 7, 11, 13, 17, 19, 23 written as escape
codes (`\a` is 7, `\v` is 11, `\r` is 13, and `\21`, `\23`, `\27` are octal
for 17, 19 and 23).  The string ends with a zero byte, so it doubles as the
end marker of the list.

6. Recursion to build a byte from bits

`int g(char*s){return*s?!(n%*s)+2*g(s+1):0;}` reads the byte out of `n`.
`!(n%*s)` is 1 if the prime divides `n` and 0 if not, and `2*g(s+1)` does
the same for the rest of the primes and shifts them one bit to the left.
It is a loop over bits written as a one line recursive function with the
`?:` operator.

7. The whole program inside a `for` header

`main` is a single `for` with an empty body.  The test `*c` stops at the
`0` at the end of `p[]`, and all of the work happens in the "increment"
part using `?:`, the comma operator and `&&` in place of `if` statements:
`n%c[1]?c+=2,0:(n=n/c[1]**c,c=p, ...)` means "if the denominator does not
divide `n`, move to the next fraction, otherwise apply it and go back to
the start".  `~n&1&&(...)` means "if `n` is even, print a byte and sleep".
The `,0` after `c+=2` is only there to make both sides of `?:` have a
usable type.

8. Confusing operator spacing

`n/c[1]**c` looks like a power operator `**`, which C does not have.  It
is `n / c[1] * *c`: divide by the denominator and multiply by the
numerator.  Dividing first keeps the value inside 64 bits.  `~n&1` is a
roundabout way to write `n % 2 == 0`.

9. Floating point argument where an integer is expected

`usleep(1e4)` passes the `double` value `1e4`, which C quietly converts to
the integer 10000 (10 milliseconds).  simple.c writes `usleep(10000)`.

10. Pointer used as program counter

`*c=p` is declared on the same line as the array, hidden after the closing
brace, along with `n=59`.  `c` walks through `p[]` two numbers at a time
(`c[0]` is the numerator, `c[1]` the denominator) and `c=p` jumps back to
the start.  Declaring several variables of an unusual type after a long
initialiser makes them easy to miss.
