# 2018/bellard

Difficulty: Hard

1. Data hidden in whitespace, braces and semicolons

The string `x` looks like random C punctuation, but it is the compressed image
written as numbers in base 1968 (123 * 16). Each number takes two or three
characters. The first character is worth 0..122: any character from 1 to 127
except the four that cannot be typed raw inside a string (`'\n'`, `'\r'`, `'"'`
and `'\\'`). That is why `Q()` computes `b-1-(b>10)-(b>13)-(b>34)-(b>92)`: each
`(b>N)` subtracts one for every forbidden character below `b`. The second part
is worth 0..15. It is either one whitespace character (space, vertical tab, tab,
form feed are 0..3), or one of `}` `;` `{` followed by a whitespace character
(4..15). The string holds real tab, vertical tab and form feed bytes, so it
looks blank or messy in an editor. In simple.c this is `refill()` with the
helpers `whitespace_value()` and `punctuation_value()`.

2. Lookup tables replaced by strange arithmetic

Instead of a table or a `switch`, `(b^8)*2%5` turns space, vertical tab, tab
and form feed into 0, 1, 2, 3, and `(b^6)%3*4` turns `}`, `;`, `{` into 0, 4,
8. XOR with a constant and then a remainder happens to give the right numbers
for just these characters. The test `s>>9` means "is `s` at least 512", which is
only a hidden way to ask "are we reading the built in string (base 1968) or
standard input (base 256)?".

3. Meaningless one letter names

The author shortened every name to one letter to save space: `Q` reads input,
`t` decodes one bit, `n` decodes a number, `R` is the inverse DCT, `W` decodes
a block and `K` writes a byte. Globals `p`, `r`, `s`, `f`, `O`, `P` are the
decoder value, the decoder range, the number base, the image width and the two
quantizer step sizes. In simple.c they are `refill`, `decode_bit`,
`decode_number`, `inverse_dct`, `decode_block`, `put_clamped`, `code`, `range`,
`base`, `width`, `luma_quant` and `chroma_quant`.

4. Loop macro

`#define e(a, d) for(a=0;a<d;a++)` hides every `for` loop behind a one letter
name. Lines like `e(H,d)e(a,d){` are really two nested loops, and `e(a,4)W(...)`
is a loop that calls `W` four times. In simple.c these are written as normal
`for` loops.

5. Magic context numbers in a flat array

The arithmetic decoder keeps two counters for each of 83 "contexts" in one flat
array `X[166]`, picked with `m=X+E*2`. The callers pass bare numbers such as
`t(3)`, `t(4)`, `n(5+B*10)`, `n(25+(B+(a<d/8)*2)*10)`, `t(61+g*2+B)`, `n(73)`
and `t(g-3)`, and nothing says what each number means. simple.c uses a two
dimensional `contexts[83][2]` array and named constants like `CTX_SIGN`,
`CTX_RUN`, `CTX_LEVEL`, `CTX_END_OF_BLOCK` and `CTX_MODE`.

6. Assignments and results used as numbers inside expressions

`if(k=p>=u)` compares and stores the bit in the same line, then `m[k]++` uses
that bit as an array index to pick which counter to increase. `k=1-2*t(3)`
turns a decoded bit into +1 or -1. `if(k=h<0)` again stores a comparison while
testing it. Each line does two or three jobs at once.

7. Short circuit hiding input reads

`if(g>5||g>2&&t(g-3))` decides whether to split a block. Because of how `||`
and `&&` stop early, the call `t(g-3)` (which reads compressed input) only
happens for sizes 8x8 to 32x32. A reader can easily miss that this condition
changes the decoder state.

8. Relying on operator precedence

Many expressions skip parentheses and depend on C's precedence rules:
`p+=b-1-...-(b>92)<<4` shifts the whole difference, `G+(1<<V-1)>>V` means
`(G + (1 << (V-1))) >> V` (rounding before shifting), and
`*i*(8-J)+i[1]*J+4>>3` rounds a weighted average. simple.c adds the
parentheses or splits these into separate steps.

9. One function doing rows and columns through strides

`R(S,Y,T,Z,U,d,V)` takes seven unnamed arguments. By changing the "step"
arguments it runs either over rows or over columns, so the same call is used
twice for a 2D transform: `R(i+d,1,i,1,c,c,10)` then `R(o,f,i+d,c,1,c,10+g)`.
The second call writes straight into the image. In simple.c the parameters are
named (`out_step`, `in_step`, `line_step`, `shift`) and a comment says which
pass is which.

10. Packed mode number

One number `q` holds the whole prediction mode: 0 is DC (average), 1..16 are
"mostly horizontal" slopes `9-q`, and 17 and above are "mostly vertical"
slopes `q-25`. The flag `C` swaps the roles of rows and columns, and
`k^C?o[h*f-1]:o[-f+h]` uses XOR to choose between the left column and the row
above. simple.c splits this into `predict_dc()` and `predict_directional()`
with variables `horizontal`, `slope` and `other_edge`.

11. Reusing variables and buffers for other jobs

`main` reuses its argument count `D` as a temporary (`D=l-L`) for the color
conversion. The coefficient buffer `i` also holds the transform's scratch
space (`i+d`), and later `i[0]` and `i[1]` hold the two reference pixels for
interpolation. `a` is a loop counter, a coefficient position and a Golomb bit
count in different places. simple.c uses a separate variable for each job.

12. Hidden constants

`acos(-1)` is used to get pi, `sqrt(2-!j)` gives the DCT scale factor (1 for
the first term and the square root of 2 for the rest), and `s=D>1?256:1968`
chooses the input source by the number of command line arguments. These are
named in simple.c (`EMBEDDED_BASE`, `STDIN_BASE`, `dct_basis()`).

13. Old style declarations

Functions have no return types (`Q(){`, `t(E){`), the parameters of `R` are
declared in K&R style after the parameter list (`R(S,Y,T,Z,U,d,V)int*S,*T;`),
and `printf`, `putchar` and `getchar` are used without `<stdio.h>`. The
compiler fills in `int` for everything left out, so the reader must guess the
types. simple.c gives every function a full prototype.
