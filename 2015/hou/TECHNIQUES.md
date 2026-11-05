# 2015/hou

Difficulty: Hard

1. Doubles used in place of every integer

`typedef volatile double T` makes nearly every variable a `double`, including
counters, flags, bytes and 32-bit words. `volatile` forces each result to be
stored as a true 64-bit double, which the rounding tricks below depend on. The
type name is hidden inside a box of comments, so `T;T` on the last line of the
box looks like nothing at all. `simple.c` uses an unsigned 32-bit `word` and
plain `int` counters.

2. Comparison done with rounding

`Q(d,l)` means `d>l` and returns 1 or 0, without using a comparison operator.
It computes `(1+(d-l)/H)/2`, which is slightly above 0.5 if `d>l`, exactly 0.5
if they are equal and slightly below 0.5 if `d<l`. `H` is 2 to the power 52. A
double that large has no room for fractions, so `+H` and `-H` round the value to
a whole number. Anything above 0.5 becomes 1, and everything else becomes 0.
`simple.c` uses `<`, `>=` and `!=`.

3. Equality done with a bell curve

`I(d,l)` means `d==l`. It returns `exp(-x*x)` where `x` is `(d-l)*C` and `C` is
at least 64. If the values match then `x` is 0 and the result is exactly 1. If
they differ, even by 1, the result is `exp(-4096)`, which is too small for a
double and becomes 0. `Y=1-I(g,EOF)` is how the program tests for end of input.

4. Division and remainder without integers

`_(d,l)` is whole-number division, using the same `+H`, `-H` rounding trick to
throw away the fraction. `f(d,l)` is `l%d`, written as `l-d*(l/d)`. The argument
order is reversed compared to `%`, and the parameters reuse the names of the
global variables `d` and `l`.

5. Choosing a value with multiplication instead of `if`

The program has no `if`, `?:`, `&&` or `||`. To choose between two values it
multiplies by a flag that is 0 or 1. `e-=w;e*=o;e+=w;` means "keep `e` if `o` is
1, otherwise set `e` to `w`". `g+=h;g*=Y;g-=h;` replaces the input byte with
the padding byte at end of input. Every statement runs on every pass of the loop,
and the flags decide which ones have any effect. `simple.c` uses `if`, `switch`
and separate functions.

6. One loop acting as the whole program

The single `while` loop reads the input, hashes it and builds the output. Flags
`u` and `E` record the current phase, so each pass does one small piece of one
phase. The loop has no real exit test. `printf` prints a string that stays empty
until the result is ready, and the loop stops when `printf` returns a non-zero
count. `simple.c` has one function for each phase, with ordinary loops.

7. Bitwise logic done one bit at a time

Doubles have no `&`, `|`, `^` or `~`. Each step of the hash therefore takes 32
passes of the loop. `l+=l`, `w+=w` and `e+=e` double each value, `F=_(l,n)` and
its neighbours take the bit that overflowed past 32 bits, and `l+=F*(1-n)` puts
that bit back at the bottom, which rotates the word. The 3 bits are combined
with arithmetic, such as multiplying for AND, and `U+=U+s` collects the result.
That one block of arithmetic covers all four logic functions, selected by the
round number in `o`. `simple.c` writes each function with bitwise operators.

8. Initial values stored already rotated

The starting values `e`, `l` and `w` do not match the ones in the RFC, because
item 7 rotates them by one bit on every pass, even while input is being read.
Reading always ends at byte 56 of a 64-byte block, so the total rotation always
leaves them 8 bits short of a full turn. The values are stored in decimal and
rotated left by 8 to make up for it: `e=1159922023` is `0x45230167`, which is
`0x67452301` rotated. `simple.c` lists the standard values in hexadecimal.

9. Constant table replaced by a formula

There is no table of 64 constants. They are worked out from
`M=cos(j*2+2);M=sqrt((1-M)/2)*n-0.5;`, a trigonometry identity for `|sin(j+1)|`
that hides the plain formula. `simple.c` calls `sin` directly.

10. Lookup tables replaced by polynomials

Rotation amounts and the order of message words are normally small tables. Here,
the lines `g=((-4*o+16)*o-12)*o;` to `g=g*Y+(((4*o-6)*o-22)*o+84);` form a
polynomial in the round `o` and the step `Y` that gives each rotation amount
times 12. `Y=(((2*o-9)*o+11)*o+1);` and the line after it give the word order.
`simple.c` has a `rotations` table and writes the word order next to each round.

11. Misleading code that looks like sound synthesis

Rotating left by `g` bits is the same as multiplying by 2 to the power `g`, which
gives `g=pow(2,(g/12))`. The rotation amounts in item 10 were multiplied by 12
only so that this line looks like the formula for musical note frequencies. With
`exp(-d*d)` looking like an envelope and `cos` like an oscillator, the program
appears to generate sound.

12. Text stored as floating point numbers

`printf("%s",(char*)(X+4))` prints the raw bytes of the array of doubles as a
string. The bytes of `4.522871578970112e217` and `7.90055321525613e115` are the
message `out of memory`, and `4.6727004911171048e-153` and
`1.6937441874215991e190` are `non-ISO rounding`. The starting value of `a` holds
the newline that ends the digest. Together with `t` it also holds ` (BE)` written
backwards, so that this text only appears on a big-endian machine. `simple.c`
uses normal strings.

13. Hexadecimal output assembled inside a double

There is no `%x` format. Each double holds 8 hex characters of the digest. `H/85`
fills the low bytes with the character `0`. Adding to `a` changes 6 of those
bytes, and `pow(2,t-304-n*o)` sets the exponent, which supplies the top 2 bytes.
`o+=Q(o,9)*39` jumps from the digit `9` to the letter `a`. `simple.c` prints each
byte with `%02x`.

14. Names that spell a phrase instead of a meaning

The globals are declared in the order `H a n d l e`, `w i t h`, `C A R E`, which
fits the box around `volatile double`. Each name was chosen for its place in the
phrase, so none of them describes what it holds. `H` is a rounding constant,
`n` is 2 to the power 32 and `C` is the position in the input.

15. Variables reused for unrelated jobs

`g` is the input byte, the position in a block, a rotation amount, an end of
block flag and a scale for the output at different points of the same pass. `o`,
`Y` and `s` are reused the same way, and `z` is the place value of a byte while
reading but the place value of a digit while printing. To follow a value you
have to trace which assignment last touched it. `simple.c` gives each job its
own named variable.

16. An array hidden behind a macro

`#define i(Y) X[(int)(Y)]` converts a double to an array index, so an array access
looks like a function call. The array itself is declared as `i(99999)={0}`, in the
middle of a list of ordinary numbers. It holds the state words, the output text
and the whole input together.
