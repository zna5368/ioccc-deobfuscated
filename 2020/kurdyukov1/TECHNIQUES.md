# 2020/kurdyukov1

Difficulty: Hard

1. Constants made from other numbers

The four starting values `0x67452301`, `0xefcdab89`, `0x98badcfe` and
`0x10325476` are never written down. `x=~i/15` is `0xffffffff / 15`, which is
`0x11111111`. Then `f=x*x-x` overflows into `0x76543210`, `a=f^x` gives
`0x67452301`, `b=f^x*9` gives `0xefcdab89`, and `c=~a`, `d=~b` flip every bit to
get the other two. `simple.c` writes the four values out as hex numbers.

2. A table built with integer maths instead of being stored

The 64 MD5 constants are normally a table of `|sin(i+1)| * 2^32`. Here `e` and
`k` are a point that is turned by 1 radian every step, using `l`, `m`, `o` and
`p` (cosine and sine of 1, split into a big part and a small part so that the
multiplications do not overflow 64 bits). `e>>40^e>>8` then makes the value
positive and shrinks it to 32 bits. `simple.c` keeps this idea but gives the
parts names like `COS_HI` and `SIN_LO` and explains them in comments.

3. Magic formulas instead of small tables

The shift amounts (`7, 12, 17, 22, 5, 9, ...`) come from
`(i|12)*152%543%82%4+i%4*43/8+4`, a formula found by trial and error that
happens to give the right numbers. The word picked from the message in each
round comes from `(x*7/2&5)-~(x*5&6)*i`, which works out to `i`, `5i+1`,
`3i+5` and `7i` for the four rounds (`-~y` is a trick for `y+1`). `simple.c`
uses a 4x4 `shifts` table and a `switch` with one case per round.

4. Comma operator and assignments hidden inside expressions

Almost all of the work is done inside `for(...)` headers joined with commas.
The outer loop condition `d=h[c=h[b=h[a=h[i=0]+=a,1]+=b,2]+=c,3]+=d,f` adds the
working values back into the state, copies them out again, resets `i`, and only
then tests `f`. The 64 rounds are in the loop body and its "increment" part,
which shuffles `a=d,d=c,c=b` and rotates `b`. `simple.c` splits this into
`read_block`, `compress_block` and plain statements.

5. Nested `?:` instead of `if` and `switch`

Reading a byte is `z<0?0:(z=getchar())>=0?g+=8,z:128`: zero after the end,
otherwise read a character, and if it is the end give `128` (the `0x80`
padding byte). The four round functions are picked with
`x>2?~d|b:x>1?b^d:x?(b^c)&d:(c^d)&~b` followed by `^c`, which is a shorter way
of writing the usual MD5 F, G, H and I functions. `simple.c` uses the
`next_byte` function and a `switch`.

6. One array used for two jobs, filled backwards

`h[20]` holds both the 4 state words (`h[0]` to `h[3]`) and the 16 message
words (`h[4]` to `h[19]`). The block is filled from the end (`h[4+--n/4]`
with `n` counting down), so message word `j` is at `h[19-j]` and the length goes
in `h[5]` and `h[4]`. `simple.c` uses separate `state` and `block` arrays
filled from the front.

7. Bytes shifted in from the top

`x=byte<<24|x>>8` slides each new byte into the top of `x`, so after four bytes
the word is in little endian order on any machine. When printing,
`h[i/8]>>(i%8*4^4)&15` uses `^4` to swap the two halves of each byte so the high
digit comes first.

8. Short meaningless names and shaped layout

Names like `h`, `f`, `g`, `z`, `e` and `k` say nothing about what they hold
(`g` is the length in bits, `z` the last character, `f` the "more blocks"
flag). The code is also shaped into a circle, so lines break in random places.

9. Windows `main` swapped by a macro

On Windows `#define main() M();int main(){setmode(...);return M();}int M()`
turns the visible `main` into a function `M` and makes a new `main` that first
puts standard input in binary mode. `simple.c` calls `setmode` at the start of
`main` inside `#ifdef _WIN32`.
