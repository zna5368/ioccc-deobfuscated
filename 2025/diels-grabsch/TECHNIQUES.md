# 2025/diels-grabsch

Difficulty: hard

1. Command line arguments used as a hidden number

`main(B)` names the argument count `B` and uses it as the starting bit count
of the output buffer. The arguments themselves are never read. Only their
number matters: with exactly 14 arguments (`p l e a s e c o m p r e s s`),
`B` is 15, so the first write sends 15/8 = 1 header byte and leaves 7 bits
waiting. Any other count gives broken output or a crash. In `simple.c` this is
`nbits = argc`.

2. File header hidden in a magic number

`p=9018655` is `0x899D1F` in hex. Read from the lowest byte up, that is the
three header bytes of a `.Z` file: `0x1F 0x9D` (the compress magic number) and
`0x89` (block mode plus a 9-bit maximum code size). Writing in decimal hides
this. In `simple.c` it is the `HEADER` macro.

3. Header bytes reused as the first "code"

After the first header byte is written, the last 16 header bits stay in `p`
and are treated as 7 waiting bits plus a 9-bit "previous code". When the first
input byte is read, this fake code is "output" like any other code, which
finishes writing the header. It also uses up dictionary slot 0 (`i` goes from
0 to 1), so a table value of 0 can safely mean "not in the dictionary".

4. One integer holding several fields

`p` holds, from the lowest bit up: the finished output bits that are not yet a
whole byte, the current 9-bit LZW code, and the newest input byte. Shifting
and XOR move values between these fields. `z=p>>B` takes the code and the byte
together as one lookup key (`byte << 9 | code`). Adding 9 to `B` turns the
current code into output bits with no copying, and the input byte becomes
the next code.

5. Everything done inside the `for` header

The loop condition `p>>=8*write(1,&p,B/8),B^17` writes the whole bytes, shifts
them out of `p` (using the byte count that `write` returns) and checks for the
end. The "increment" part reads a byte and does the whole dictionary
lookup/insert. The loop body `B%=8` only throws away the bit count of the bytes
just written.

6. Nested conditional (`?:`) operators instead of `if` statements

`p<0?B=17:j[z=p>>B]?...:(...)` is an `if / else if / else` chain: end of
input, string found in the dictionary, or a new string. `i>>8?0:i++` means
"if the dictionary is full store 0, else store the next number".

7. XOR used for setting and clearing bits

`p^=getchar()<<9+B` puts the byte into empty (zero) bits, so XOR acts like OR.
`p^=(z^z[j]^256)<<B` XORs the old field value `z` away and puts the new code
`j[z]^256` in its place, in one step. At end of input `getchar()` returns -1, and
`-1<<9+B` sets every high bit, so `p` becomes negative. That is how `p<0`
detects the end, and those 1 bits are the padding in the last two bytes.

8. Codes stored with an offset hidden by XOR

The dictionary stores the numbers 1..255, but the real LZW codes are 257..511.
`j[z]^256` adds 256 using XOR, which works because the stored number is always
below 256. Code 256 (the CLEAR code in block mode) is never used.

9. Reversed array subscript

`z[j]` is the same as `j[z]` because `a[b]` means `*(a+b)`.

10. Implicit `int` and missing declarations

`z,i,p` and `j[...]` have no type, so they are `int` by default in old C.
`main(B)` has no types either, and `write` and `getchar` are used without any
`#include`, so the compiler guesses their declarations.

11. Memory layout tricks

`write(1,&p,B/8)` writes the raw bytes of the `int` `p`. This only gives the
right bytes on a little-endian machine, where the lowest byte comes first.
`simple.c` writes each byte one at a time instead.

12. Shift precedence

`getchar()<<9+B` is `getchar() << (9+B)` because `+` binds tighter than `<<`.
Leaving out the brackets makes the expression harder to read.
