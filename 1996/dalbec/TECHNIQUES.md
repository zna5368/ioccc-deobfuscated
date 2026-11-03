# 1996/dalbec

Difficulty: Hard

1. Easily confused identifiers

The only names in the program are `I`, `O`, `O0`, `OO` and `l`. The capital
letter `I`, the lowercase `l` and the digit `1` look alike, and so do the letter
`O` and the digit `0`. So `OO>>!!OO` or `I-I/I-I/I` are hard to read at a
glance. In `simple.c` they became `n`, `base`, `exp`, `half` and so on.

2. One function does everything

`main()` is the only function. It decides which job to do by looking at its
first argument `I` (which starts out as `argc`):

- `I >= 2`: there are command line arguments, so read the base from `argv[1]`
- `I == 1`: there are no arguments, so use base 2
- `I == 0`: compute `base^exp mod n` (modular power)
- `I == -1`: compute `a*b mod n` (modular multiply)
- `I == -2`: compute `(a+b) mod n` (modular add)
- `I <= -3`: test the odd number `n = -I` and then move on to `n + 2`

`main()` also takes three extra parameters (`O0`, `OO`, `l`) that a normal
`main()` never has. Depending on the job they hold the base, the exponent, the
modulus, or a counter. `simple.c` splits these jobs into `add_mod`, `mul_mod`,
`pow_mod` and a `main` with a loop.

3. Comparisons that hide which value is being tested

The jobs above are picked by tests such as `!!I>=I`, `!I>=I`, `!!~I>=~I` and
`!~I>=~I`. Each one is true for some range of `I`. For example, `!!I` is 0 or
1, so `!!I>=I` is only false when `I` is 2 or more. `~I` is `-I-1`, so the
tests that use `~I` check whether `I` is -1, -2, or less than that. Nothing
shows that these are really `I >= 2`, `I == 1`, `I == 0`, `I == -1` and so on.

4. Hiding constants with `!`, `/` and subtraction

Small numbers are never written out. Instead they are built from `I`:

- `!I` is 1 when `I` is 0, and 0 otherwise. `!!I` is the opposite.
- `I/I` and `OO/OO` are always 1.
- `I-I-I` is `-I`.
- `I-!I-!!I` is always `I-1` (only one of `!I` and `!!I` is 1).
- `~!!I-!!I` with `I >= 2` is -3. `I+I-I+I` with `I == 1` is 2.
- `OO>>!!OO` is `OO>>1`, because `OO` is never 0 at that point.

The README calls this "substituting `!a` for `0` and `1`".

5. Implicit `!= 0` and inverted conditions

Tests like `!(OO-!I||I)` mean "`OO == 1` and `I == 0`", and
`!(OO-l+!I||I)` means "`OO == l-1` and `I == 0`". A subtraction is non-zero
when two values are different, so `!(x - y)` is a hidden `x == y`. The extra
`||I` makes the whole test false for the multiply job, so the same code acts
differently for powers and for multiplication.

6. Whole program is one expression

The body is one `return` statement made of nested `?:` operators and comma
operators. Assignments to the parameter `OO` happen inside the expression, and
there are no `if`, `for` or `while` keywords. Code formatters cannot lay it out
in a readable way. In `simple.c` these became `if`/`else` blocks and
local variables.

7. Recursion used as a loop

The main loop (`I <= -3`) calls `main()` again with `I-2` (the next odd
number) and `OO+1` (half of that number minus one). The compiler turns this
call into a jump, so it runs forever like a loop. The prime test and the
`printf` are hidden inside the expression that computes the fifth argument,
whose value is never used. `simple.c` uses a plain `for` loop instead.

8. Arithmetic built from recursion

Multiplication is done by doubling and adding (`(a+b)%n` over and over). Powers
are done by squaring and multiplying. Both use the same recursive branch of
`main()`. The only difference is whether `I` is 0 or -1, which picks whether the
"add" or "multiply" job is called next.

9. Hidden strong probable prime test

The power function has two extra rules: a result of 1 after an odd step is
changed to `n-1`, and a result of `n-1` stays `n-1` instead of being squared.
Together with checking `base^((n-1)/2) == n-1`, this makes a strong
pseudoprime test. That is why composite numbers like 2047 (base 2) and 121
(base 3) are printed. None of this is visible in the code, because the rules
look like normal parts of the nested conditional.

10. Old style function definition

The original uses `main(I,O,O0,OO,l)` with the parameter types declared below
the header, and gives `main` five parameters. Only `argc` and `argv` are really
passed in, so the other three start as garbage, but they are filled in before
anything reads them.
