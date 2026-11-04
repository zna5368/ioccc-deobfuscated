# 2011/vik

Difficulty: Hard

1. Lookup table hidden in a giant buffer

`char E[1<<25]="  ; } }  { { } ..."` is the 32MB buffer the MOD file is
read into, but it starts out holding a string. Before the file is read,
the first loop turns every 10 characters of the string into one number:
a space is worth 0, `{` is 1, `}` is 2 and `;` is 3, and each value is
shifted left by its position (0 to 9) and added up. This creates the
table of Amiga note periods `n[]` (856, 808, 762, ...). Only ten bits
fit, so the first nine periods lose 1024. Since the rest of the 32MB
array is zero, the compiler has to write out the whole array, which is
why the assembly file is so large. The simple version writes the table
as a normal `int` array.

2. Single-letter macros for syntax

`O`, `N` and `M` expand to `[s]`, `[v]` and `[r]`, so `8 O` becomes
`8[s]`, which is the same as `s[8]` in C (`a[b]` means `*(a+b)`, so the
order does not matter). `C` is `++`, `o` is `for(`, `B` is `,putchar`,
`d` is `u==`, `P` is `2 M==` (the effect number), `m` is `:0` and `g`
is `:0,`. Code like `P 14?W(r,3)...m m)` is a chain of `?:` operators
made out of these pieces.

3. Magic array indexes instead of variables

Each channel's state is the 50 ints at `s = e+200+50*i`. `0 O` is the
volume, `8 O` is the period, `32 O` the note, `34 O` the sample number,
`37 O`/`38 O` the pattern loop row and count, and `1 O` to `31 O` hold
the last parameter of each effect (`2 M O = 3 M` stores
`s[effect] = param`). The mixer state is `v = b+7*i` (`0 N` position,
`1 N` phase, `3 N` end, `4 N` loop length, `5 N` step, `6 N` volume),
sample headers are 5 ints each in `e`, and pattern cells are 4 ints each
in `L`. The simple version uses structs and named `enum` indexes.

4. Whole program in `for` headers and `?:` chains

Almost every statement is inside the header of a `for` loop whose body
is empty (`;`). For example, the per-channel effect processing is the
increment part of `o i=0; i<R; ... i C);`, and the sample output is the
increment part of the loop that counts down `j=F`. Instead of `if`
statements, the code uses `a ? b : 0` joined with commas. This saves
space but hides which code runs when.

5. Macros with missing parentheses

`Y(a,b)` and `Z(a,b)` are min and max, and `X(x,a,b)` is a clamp, but
none of them put parentheses around the whole result. When they are used
after `?` or `=`, the C precedence rules decide how they combine with the
code around them. In the tone portamento code
`8 O>l?8 O=Z(8 O-3 O,l):8 O<l?Y(8 O+3 O,l) m`, the second branch is
missing `8 O=`, so the slide toward a lower pitch is computed and thrown
away. The simple version keeps this behaviour and notes it in a comment.

6. Macros that build trigger code

`x` is a large macro that starts a sample on a channel: it sets the
period from the note, points the channel at the sample data, sets the
loop end and length, and applies the sample offset effect. `y(o)` sets the
period with an arpeggio offset, `z(o)` looks up the note's period, `W(t,i)`
splits a byte into its two 4-bit halves `u` and `w`, and `S(a)` reads a
big-endian 16-bit word. These are used in many places, so after
preprocessing the same code appears several times. The simple version
turns them into functions.

7. Strings hidden in macros

`D(b,i)` turns its argument into a string with `#b`, so `D(MK,0)` checks
`T[0]=='M' && T[2]=='K'`. This is how the file type is detected from the
`M.K.`, `FLT4`/`FLT8` and `6CHN`/`8CHN` signatures, without writing
those strings in the code.

8. Arithmetic in place of named constants

`110250/l` is the tempo (44100 samples per second times 2.5 divided by
the beats per minute), `F=882` is 44100/50 samples per tick, `13200` and
`999*8 O` set the playback speed of the sample from its period, and
`k=1<<(i&2)` with `k^5` picks the left and right volume (1 or 1/4) so
channels follow the Amiga left, right, right, left layout.

9. Unsequenced sum that depends on the compiler

`Q(1)Q(4)Q(12)Q(12)Q(4)Q(1)0` is meant to step through the sample six
times and add each value with the weights 1, 4, 12, 12, 4, 1. All six
steps change the same variables inside one `+` expression, and C does
not say in which order the parts of a `+` are done. GCC does all six
steps first and then reads the same sample six times, while clang
interleaves them and produces different audio. The simple version
copies GCC's order.

10. Dead or misdirected code

`(35 O-24)/256` adds the finetune to the note, but the finetune is
always between -16 and 7, so the result is always 0. The note delay
effect checks `I==e[29]` (the loop length of sample 5) instead of its
own parameter `29 O`. The program also ignores the return value of
`fopen`, so it crashes without a file argument.
