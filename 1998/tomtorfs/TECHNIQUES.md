# 1998/tomtorfs

Difficulty: Easy

1. Reversed array subscripts

Every array access is written backwards, such as `7[b]`, `5[b]` and `1[A]`.
In C, `x[y]` is defined as `*(x + y)`, and since addition does not care about
order, `7[b]` is exactly the same as `b[7]`. It works, but it looks strange and
hides which variable is being used.

2. One array instead of many named variables

All the program's values live in one array `C b[8]`: `b[0]` is the CRC width,
`b[1]` the polynomial, `b[2]` the reflected flag, `b[3]` the initial value,
`b[4]` the final XOR value, `b[5]` the CRC, `b[6]` the current byte and `b[7]`
the loop counter. Without names, the reader has to remember what each number
means. simple.c gives each one its own variable such as `width`, `poly` and `crc`.

3. Meaningless single letter names

`a` is `argc`, `A` is `argv`, `B` is the `FILE *` and `C` is a `typedef` for
`unsigned long`. The upper and lower case pairs (`a`/`A`, `b`/`B`) make them
easy to mix up.

4. Argument parsing folded into a loop with a computed base

`for(7[b]=0;7[b]<5;7[b]++)b[7[b]]=strtoul(A[2+7[b]],0,16-!7[b]*6);` reads all
five numeric arguments in one loop. The base is `16-!i*6`: `!i` is 1 only when
`i` is 0, so the first argument (the width) is read in base 10 and the rest in
base 16. simple.c uses five separate `strtoul` calls with a plain base.

5. Macro hiding a return statement

`#define exit(a) return EXIT_FAILURE` makes `exit(1)` look like a call to the
library `exit()` function, but it is really a `return` from `main`. simple.c
just writes `return EXIT_FAILURE;`.

6. Combined condition with assignment inside

`if(!(a==7&&(B=fopen(1[A],"rb"))))` checks the argument count, opens the file
and stores the file pointer all in one expression. The `&&` makes sure the file
is only opened if there are enough arguments. simple.c writes it as
`argc != 7 || (file = fopen(...)) == NULL`.

7. Bit reversal by swapping differing bits

The loops `if(((x>>i)^(x>>(n-1-i)))&1) x^=(1<<i)^(1<<(n-1-i));` reverse the
order of bits. They XOR bit `i` with its mirror bit; if the two differ, flipping
both of them swaps them (if they are the same, swapping changes nothing). This
is done twice, once for each 8-bit input byte and once for the final CRC, so
simple.c moves it into a single `reflect()` helper function.

8. Mask that avoids an oversized shift

`((((C)1<<(0[b]-1))-1)<<1)|1` builds a mask of the lowest `width` bits. The
simpler `(1<<width)-1` would shift by 64 when `width` is 64, which is undefined
in C, so the code shifts by `width-1` and then shifts once more by hand.

9. Shaped source and cramped layout

The code is laid out as a large picture made of text, with no indentation, splitting
tokens like `5[` / `b]` across lines. Running a code formatter (as the author
suggests) restores normal structure.
