# 1990/jaw

Difficulty: Hard

1. Keyword-hiding macros

`#define I ;if(`, `#define W ;while(`, `#define X return 0`, `#define G getchar()`,
`#define P putchar`, `#define C char` and `#define Q 256` replace keywords and
common calls with single capital letters. `I` and `W` even carry the
semicolon that ends the *previous* statement, so code like `k=4 W g<k)` is
really `k=4; while (g<k)`. Without running the preprocessor the code hardly
looks like C.

2. Returning through the comma operator

`X` is `return 0`, but it is nearly always followed by a comma: `X,a()` is
`return 0, a();`, and the comma operator throws away the `0` and returns
`a()`. The same trick makes `X-1` return -1 and `X,1` return 1. The error
macro `F` is `X,fprintf(stderr,"oops\n"),1`, which prints a message and
returns 1 in a single expression.

3. One program, two tools picked by `argv[0]`

`I**V-97)X,a()` checks the first letter of the program's name: if it is not
`a` (ASCII 97) the program runs the `zcat` decompressor `a()`, otherwise it
falls through to the `atob` decoder in `main`. The Makefile builds `zcat`
and then links the same file to the name `atob`.

4. Recycled global variables

The same one-letter globals mean different things in each tool. In `atob`,
`w` is the base-85 group being built, `k` counts digits, and `o`, `m`, `f`
are the XOR, sum and rotate checksums. In `zcat`, `w` is the current code,
`k` is the maximum code width, `o` is the previous code, `m` is the largest
code for the current width and `f` is the next free dictionary slot. Even
`g` is a function (the code reader), the `argc` parameter of `main` reused
to hold characters, and the parameter of `b()`.

5. Subtraction and addition in place of comparisons

Tests are written as arithmetic that is zero only when the values match:
`G-10` means "character is not a newline", `getchar()-78` means "not `N`",
`(g=G)+1` means "not EOF (-1)", `w=g())+1` means "code is not -1" and
`scanf(...)-5` means "did not read 5 values".

6. Bitwise operators instead of logical ones

`g>32&g<'v'`, `g-'z'|k`, `w==Q&e` and `m<f&n<k` use `&` and `|` where `&&`
and `||` are meant. This works because comparisons give 0 or 1. The final
`atob` check ORs four differences together, `(M-N|c-o|s-m|r-f)&4294967295`,
so one test catches a wrong length or any wrong checksum; the mask
(`0xffffffff` written in decimal) keeps only the low 32 bits so it also
works when `long` is 64 bits.

7. Magic numbers

Characters and sizes appear as plain numbers: 97 is `'a'`, 78 is `'N'`,
10 is `'\n'`, 33 is `'!'` (the first base-85 digit), 31 masks the code
width from the header byte, `k>>7&1` is the "block mode" flag, 69001 is the
hash table size borrowed from `compress`, and `Q*Q` is a 64K buffer.

8. Side effects buried in a condition

In the code reader `g()`, the test
`I m<f&n<k&&(m=(1L<<++n)-1)||O>=S)` both checks whether the code width must
grow and, inside the condition, increases the width `n` and the mask `m`.
The assignment is always non-zero, so it doubles as "true, read a new
chunk". The rewrite splits this into a plain `if` that grows the width
and sets a `refill` flag.

9. Compressed bit-extraction expression

`(1<<8-q)-1&*p>>q|m&((15<n+q)*p[2]*Q|p[1]&255)<<8-q` pulls one code of `n`
bits starting at bit `q` of the buffer. It depends on operator precedence
(`-` before `<<` before `&` before `|`) instead of parentheses, uses
`(1<<8-q)-1` to undo sign extension of a signed `char`, and multiplies by
the comparison `(15<n+q)` (0 or 1) to decide whether a third byte is
needed.

10. Nested calls that reuse return values

`y(P((C)(w>>--k*8)&255))` writes a byte with `putchar` and passes
`putchar`'s return value (the byte just written) straight into the checksum
function `y`, while `--k` inside the shift also counts down the loop.

11. One array, two jobs

The array `D` is the input buffer for the code reader (`fread(D,1,n,stdin)`
fills its first bytes) and also the stack used to reverse a decoded string
(`C*p=D+Q` starts the stack 256 bytes in). The rewrite names it `buffer`
and explains both uses.

12. Hidden padding arithmetic

`b(g=3-(M-1&3))` turns the file length `M` into the number of zero bytes
that padded the last 4-byte group. The same `g` is passed to `b()` as the
point to stop writing and then counted down by `W g--)y(0)` so the padding
still goes into the checksums.

13. Do-everything `scanf`

`scanf("%ld%lx E%lx S%lx R%lx ",...)` parses the whole trailer line
`N <len> <hexlen> E <xor> S <sum> R <rot>` in one call: the literal letters
in the format are skipped over, and the trailing space eats any following
whitespace. The preceding `W G-78);` skips everything up to the `N`.

14. Old-style declarations and implicit `int`

Functions such as `y(l)`, `b(g)` and `a()` have no return types or
parameter types, so they default to `int`, and `main(g,V)C**V;` uses the
pre-ANSI style of declaring parameter types after the parentheses.

15. Packed layout

All statements are run together into a few dense lines with no
indentation, and statements are joined with commas (`O=0;S=...`,
`t[w]=0,h[w]=w`) so loop bodies need no braces.
