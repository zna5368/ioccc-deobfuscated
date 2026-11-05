# 2015/dogon

Difficulty: Insane

1. Three numbers packed into one integer

`i+=d` looks like one addition of two counters, but each `unsigned long long`
holds three 20 bit numbers side by side, so this moves a point in x, y and z at
the same time. The `P` macro builds these values by shifting each part into place
with `T`, and the `X` macro pulls the top 6 bits out of each part with
`q>>10&63|q>>24&4032|q>>38&258048`. Nothing in the types tells you this is
happening. `simple.c` calls this type `Packed`, describes the layout at the top
of the file, and uses `pack_vector` and `voxel_index`.

2. Negative numbers inside unsigned lanes

`T` shifts a signed `long long`, so a negative part borrows from the part above
it, even though the variables are declared `unsigned`. The result is still right
as long as the final numbers are positive, but you cannot read a part on its own
to see its value. This is why the start value `q=0x820008202625a0` looks wrong:
its lowest part is too big for 20 bits and spills into the next one.

3. Colours packed with gaps for multiplying

Colours are stored out of the usual order, with blue, red and green at bits 0, 16
and 32. The gaps mean that `o=o*b>>8` dims all three at once, because a channel
that grows during the multiplication does not reach the next one. Then
`G[L]=o>>32<<8|o&16711935` squeezes them back to the normal layout. `simple.c`
does this once, in `draw_pixel`.

4. A record hidden in an integer

Each element of `z` is a structure in disguise. `3*(Q<=m)|f|b<<56` and
`f=n|l*4|x<<32` put a face, a voxel number, a texture position and a brightness
in one number, and tests such as `(int)r`, `!(h-3&3)`, `h<<8>>40` and `h>>56` take
pieces back out. The mask `258063&r>>38` checks two separate groups of bits in a
single step. `simple.c` documents the fields and gives each piece a name.

5. Macros that hide the syntax

`#define So long` makes `unsigned So long` mean `unsigned long long`, and `R` is a
call to `rand`. `y(n,L)` is the sneakiest: it expands to `for(n=0; n<L 3; n++)`,
with the `3` after the second argument. So `y(l,)` counts to 3, but `y(k,p+)`
counts to `p+3`, because the `+` sits in front of the `3`. `P(v,L)` is more than
one statement and uses `l[v]`, which means `v[l]`. It also changes `d` and `l`
without saying so.

6. Variables that look like they belong to the program

`W` and `p` are never defined in the source. They come from the Makefile as
`-DW=1024 -Dp=800`, and read like ordinary variables, even though `p` is the
height. `simple.c` gives them the names `WINDOW_WIDTH` and `WINDOW_HEIGHT`.

7. Single letter globals with several jobs

Almost everything is a global with one letter. `i` is a column of the screen, then
the position of a ray, then a texture column. `d` is a packed vector and also a
loop counter in `y(d,)`. `b` is a direction flag inside a ray, then a brightness
for a pixel, and a brightness that must survive from one texel to the next in the
world generator. `F`, `l`, `t`, `r`, `h` and `o` are all reused in the same way.
`simple.c` makes each job its own local variable.

8. A local that hides a global

`float a[6]` in the middle of the render code hides the global `a`, which holds the
last key and is the loop's exit test. In the world generator, the global `a` is a
colour. So `a` means three things, and which one depends on where you are.

9. Two programs sharing one loop

`if(!H)` splits the main loop into the game and the world generator. `H` starts at
`1<<18` and each trip around the loop does `--H`, so the loop runs as a generator
262144 times before it starts to act as the game. There is no sign of this
structure unless you notice that `H` is both a counter and a state. `simple.c`
calls `generate_world` once before the loop.

10. Interleaved loops

`F=k%w*4|k/w` and `i=_%I*4|_/I` turn plain counters into a shuffled order: every
fourth row or column comes first, then the ones between them. The `+3` in the loop
limits and the `F<p&i<W` test are needed to make the shuffle land on every pixel.
The order matters, since a later pixel reads the results of earlier ones.

11. Key codes tested with bit tricks

`*(a&1?&C:&B)-=(.05 -a/2%2*.1)*!(a-1&4092^3920)` handles all four arrow keys
without naming one. The lowest bit picks which angle to change by choosing a
pointer, the next bit picks the sign, and the last part multiplies by 0 unless the
key is an arrow, so that other keys do nothing. In the same way
`a+2&0xfe0^0xfc0||(s=a+2&31)` turns F1 to F32 into a speed. Here `^` is used to
mean "not equal to". `simple.c` uses a `switch` on named key codes.

12. A polynomial in place of a table

`3352537*L*L-14202379*L+19205553` looks like a calculation, but it is a curve fitted
so that the values for `L` of 1, 2 and 3 are the colours `0x7F7F7F`, `0x4040FF` and
`0x675231`. `simple.c` uses a small table.

13. Colours and masks written in decimal

Numbers like `9858122`, `16711935` and `4190208` mean nothing in decimal, but
`0x966C4A`, `0xFF00FF` and `0x3FF000` are easy to recognise as a colour and two bit
masks. `simple.c` writes them in hexadecimal with names. The block numbers 3, 4, 5
and 8 have no names in the original either, and hide in tests such as `L-8?` that
mean "if not 8".

14. Side effects in unexpected places

`DefaultGC(j,J[X(q)]=0)` looks like it only asks for a graphics context, but the
assignment inside it erases the block at the player's position, and its value is
the screen number. Similar cases are the assignment in
`D[...]*(b=h>>56)>>8`, `if(t=J[l])` and `if(h=D[...])`. `XMapWindow` is run in the
first part of a `for` that has nothing to do with it. `simple.c` puts each of these
on its own line.

15. Bitwise and logic operators used as control flow

`t||(A=d)` is an `if`. `L-1|!(R%3)` uses `|` rather than `||` on purpose, so that
`rand` is always called even when the left side is true. Skipping that call would
change every random number after it. `(i-1&15)<14&(F-1&15)<14&!(F&16)` uses `&` to
join tests, and `~-b` and `+!b` make a value either stay the same or be negated
without an `if`.

16. Precedence puzzles

`t=r=l^(l^l-(1<<6*n))&63<<6*n` relies on `-` and `<<` happening before `&`, and `&`
before `^`. It uses the trick `a^(a^b)&mask`, which takes the bits in `mask` from `b`
and the others from `a`, to subtract 1 from one 6 bit part with wrap around.
`simple.c` has this in `voxel_below`.

17. Two values mixed with OR

`(i|i>>40)&1023` looks like a mistake, as it joins the x and z parts. It works
because a ray that hits a face has a fraction of 0 along that axis, so OR gives
the other one, and one expression serves two kinds of faces.

18. Branchless arithmetic

`_^=_>>31` flips the bits of negative numbers, which is almost an absolute value.
It looks like noise, and is here to draw the rings on the end of a log.

19. Reading past the end of an array

`f=z[L+(4-l)*o]` goes past the end of `z` for the last rows of the screen. With
this compiler `D` comes straight after `z` in memory, so the program really reads
texture data as if it were a pixel. It also reads across the end of a row into the
start of the next one, which may be left over from the previous frame. Both only
work out because a mismatch just means the pixel is calculated in full. `simple.c`
keeps both arrays in one so that this read is well defined.

20. Code that depends on the compiler

`R%16*(R%4<...)` calls `rand` twice in one expression. C does not say which call
goes first, so the world depends on the choice of the compiler. GCC goes left to
right here, and `simple.c` uses separate statements to make that clear.

21. Depending on unsigned wrap around

In `l>F-19`, the value of `F-19` can go below zero, and since it is unsigned it turns
into a huge number and the test fails. Written with signed numbers, the test would
be true for that one case and a few texels in that row would change.

22. Misleading layout

The code is shaped into a large letter, so lines break in the middle of names and
expressions, such as `*(` at the end of one line and `a&1?&C:&` on the next, and
long gaps sit inside expressions. Stray changes like `u+0` add more noise. You
need to ignore the layout and read the code one token at a time.
