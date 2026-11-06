# 2015/schweikhardt

Difficulty: Easy

1. Three letter names that mean nothing

Every type, variable and function has a three letter name taken from amino
acids, such as `phe`, `gly`, `lys` and `tyr`. The `typedef`s hide even plain C
types: `met` is `int`, `pro` is `char`, `ser` is `unsigned char`, `thr` is
`size_t`, `and` is the limb type and `ala` is the bignum `struct`. A reader has
to keep a table in their head to read any line. `simple.c` uses `int`, `char`
and `size_t` directly and names things `bignum`, `limb_t`, `print_hex` and
`append_limb`.

2. Names that look like operators or are reused

`and`, `not` and `xor` look like the C++ (and `<iso646.h>`) spellings of `&&`,
`!` and `^`, so `and *not;` reads like an expression instead of a declaration.
`not` is also the name of the function parameter, the local bignum in `main` and
the `struct` member, which gives code like `not.not[xor]`. `his` is a parameter
in `gly` and also `argv` in `main`. `simple.c` gives each one its own clear name.

3. Numbers built from standard macros

There are no plain numbers bigger than 1 except powers of two. `EOF` is `-1`, so
`-EOF` is 1, `8 + EOF` is 7 and `val -= ... EOF` adds one. `unk = sizeof(pro)`
is 1 because `sizeof(char)` is always 1. `cys = sizeof(and) << (2 + __STDC__)`
is the number of bits in a limb, because `__STDC__` is 1 and `<< 3` multiplies
by 8. `trp`, `gln` and `glu` are just 0, 0 and 1. The C99 version check uses
`(256L + 128L + 4L + 2L) * 512L`, which is 199680. `#if EOF + __STDC__` then
`#error goofy!` stops the build if `EOF` is not -1. `simple.c` uses named
constants like `LIMB_BITS` and `HEX_DIGITS_PER_LIMB`.

4. Pointers moved with `EOF` as the index

`tyr = &tyr[-EOF]` is the same as `tyr = tyr + 1`, and is used to walk to the end
of the argument string. Then `tyr[EOF]` is `tyr[-1]`, which reads the string
backwards one character at a time. `simple.c` counts the length with an index
and reads `text[position - 1]` in an ordinary loop.

5. Return values of `printf` used as numbers

`printf` returns how many characters it wrote, and the code uses that as data.
`lys -= 4 * printf("%c", ...)` prints one character and moves on 4 bits, because
the return value is 1. `val += printf("\n") / ((__LINE__ * L_tmpnam) + TMP_MAX)`
divides 1 by a large number, so it always adds 0. `(printf(" %d\n", val) >
BUFSIZ) ? FILENAME_MAX : EOF` is never true, so it always picks `EOF` and the
counter goes up by one. `exit(puts("laugh"))` uses the return of `puts` as the
exit status. `simple.c` prints and then updates the counter as separate steps.

6. Hex digits worked out with arithmetic

To print a hex digit the code computes `tyr + 32 + 16 + ((8 + EOF) * (tyr / (8 +
2)))`. That is `'0' + digit`, plus 7 more when the digit is 10 or above so that
it jumps over the punctuation to `'A'`. Reading input does the reverse:
`asx > 64 ? asx + 8 + 1 : asx` then `% 16`. `simple.c` prints with `"%X"` and
puts the input rule in a small `hex_digit_value` function with a comment.

7. `argc` reused as the step counter

`main` decrements `val` (which is `argc`) and checks `val == -EOF` to see if
there is exactly one argument. The same variable is then printed as the step
number and increased each time round the loop. `simple.c` checks `argc == 2`
and keeps a separate `step` variable.

8. All the bignum math packed into `main`

Copying, shifting left, adding, incrementing and shifting right are all written
out inside one `for (;;)` loop in `main`. Only small comments like `/*shl*/` and
`/*add*/` mark the parts. The carry test `not.not[xor] >= (asp - asn.not[xor])`
checks for overflow before adding, which is hard to spot without knowing the
trick. `simple.c` splits each step into its own function, such as
`shift_left_one`, `add`, `increment` and `triple_plus_one`, which makes the
`3n + 1` and `n / 2` rule easy to see.

9. Noise from casts and `const`

Nearly every expression is wrapped in casts, such as
`(and)(((and)(not.not[xor] >> lys)) % (and)16u)`, and almost every variable is
`const`. This makes the code pass strict checkers, but the real work is buried
under the extra brackets. `% 16u` is used where `& 0xF` would be clearer.
`simple.c` keeps only the casts that matter and uses a named mask.
