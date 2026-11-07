# 2025/jingp49

Difficulty: Medium

1. Shaped source layout

The whole program is squeezed into a box shape (a police box), and the
comments `P O L I C E  P U B L I C  B O X`, `TELEPHONE`, `FREE FOR USE OF
PUBLIC` and `PULL TO OPEN` are written as signs on its door. Statements get split
across lines in odd places, like `srand` on one line and `(y)` on the next, and
extra spaces sit inside tokens such as `s P` or `++    i`. The compiler ignores
whitespace and comments, so all of this is decoration that hides the structure.

2. Macros as abbreviations

`#define u8 unsigned char`, `I int`, `R rand()`, `M (u8*)malloc(a)` and
`O(a) printf("%c", a)` make short names for common pieces of code. When you read
the code you have to swap each short name back for what it stands for in your
head, so ordinary statements look strange.

3. Hidden array index macro

`#define P [i*D+j]` turns the text `s P` into `s[i*D+j]`. The square brackets
never show up in the program body, so it is hard to tell that `s`, `t`, `u`,
`v`, `h` and `g` are 2D screen buffers stored row by row in 1D arrays.

4. Logic hidden in a macro

`N` contains a whole `if/else` chain that moves a brightness level up or down at
random. It is used as a single letter, `k=t P;r=R;N;s P=(u8)k;`, and works only
because it quietly reads and writes the variables `k` and `r` around it. It also
has a typo, `k%10000<7` where `r` was meant. Because `k` is 0 there, the test is
always true, so black cells always get brighter by one step.

5. Single letter, meaningless names

`D` is the width, `W` the height, `s`/`u` the two feedback pictures, `t`/`v`
their zoomed copies, `h`/`g` the current and next logo, `o` the cross fade amount
and `e` the zoom factor. The names `eg` and `ql` are first used for the logo
margins, then reused for the zoom margins. The width and height use `D` and
`W`, which you might read the wrong way round.

6. Encoded strings and bitmaps

The brightness ramp `"  ,:;!?{$%#@"` is written as the decimal numbers
`{32,32,44,58,...}`. The "DOCTOR WHO" logo is packed 8 pixels per byte into
`l[45]` (`206,238,238,...`). A loop unpacks it into `q` with shifts and masks:
`(l[j/8+i*3]>>(7-j%8))&1` picks out bit `j` of row `i`.

7. Hex and decimal mixed for the same timeline

The frame number `c` is compared against both `0x6E`, `0x78`, `0x8c`, `0xaa`,
`0x118` and their decimal versions `110`, `120`, `140`, `170`, `280`. This hides
that only a few scene boundaries are used over and over.

8. One giant frame loop with overlapping time ranges

Each frame runs several `if` tests with overlapping ranges, like
`c<0x6E||(c>0x78&&c<170)` and `(c>70&&c<140)||(c>170&&c<280)`, and then one long
`else if` chain decides what to draw. Some frames (89, 120, 140, 170) draw
nothing and only reset buffers. You have to lay all the ranges out side by side
to see the scenes: the first tunnel, a fade, a mirrored tunnel, the logo
flashing in, and the logo shrinking.

9. Loop style swapping

As the author says, every `for` loop that walks over the buffers is written as
a `while` loop with the counter set and stepped by hand
(`i=0;while(i<W){j=0;...++j;}++i;}`). The animation loop, which would usually be
a `while`, is written as `for(;c<280;++c)` and starts from a value set much
earlier. The loop bounds are hard to spot because they sit far from the loop
body.

10. Uninitialized random seed

`srand(y)` seeds the random number generator with `y`, which is never given a
value. Whatever happens to be in that memory becomes the seed, in place of the
usual `time(0)`. The warning that would point this out is turned off in the
Makefile with `-Wno-uninitialized`.

11. Dead stores and empty statements

`a=4;k=3;` and the `a=0;` inside the bitmap loop store values that are never
used. There are also stray `;` (`};`, `{;i=0;`). They are noise to make the
reader wonder what they are for.
