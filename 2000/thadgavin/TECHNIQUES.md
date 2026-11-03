# 2000/thadgavin

Difficulty: Medium

1. ASCII art layout

The whole source is laid out as a picture, so statements, macros and even
`#include` lines are broken up and indented at random places. Lines like
`(s,0,0,0,        0); } else {  SDL_Init(` hide where one statement ends and
the next begins. The compiler ignores the extra spaces and newlines, so the
picture costs nothing at run time.

2. One-letter global variables with many jobs

Everything is a global with a one-letter name: `X`/`Y` are the screen size,
`P` holds the precomputed pixel coordinates, `l` holds three palettes back to
back (`l`, `l+j` and `l+j*2`), `U` is the pixel buffer and `C[5]` packs five
unrelated pattern settings into one array. `j=1024` is used both as the
palette size in bytes and as the modulus of the random number generator. Some
variables, like `x`, `y` and `n`, are reused for unrelated loops in different
functions, and `c` is never used at all.

3. Macros that hide statements

`R1(t)` expands to `t=(int)(t*123456789)%j; t/=j;`, a small pseudo-random
number generator. `Rl(C,t)` stores one color channel, and `RI(C)` flips the
fade direction, makes three new random numbers and fills a whole palette in a
loop. Because the macros end in semicolons and loops, the line
`if ((p+=B)>1){p=2-p;RI(l+j)}` looks like one call but is really four
statements and a `for` loop. `RI` also names its parameter `C`, the same name
as the global array `C`, which makes it look like it uses that array.

4. Token pasting to build function names

On DOS the macro `Q(u,v)` is `u##portb(0x3##v`, so `Q(out,C9)` becomes
`outportb(0x3C9` and `Q(in,DA)` becomes `inportb(0x3DA`. The `##` operator
glues tokens together, so neither the function names nor the VGA port
numbers appear in the source. The macro `W` is `; Q(out,C9),*h++/4)` and is
written three times in a row (`W W W`) to send red, green and blue.

5. Array indexing written backwards

`n++[C] = t*n/12` means `C[n++] = t*n/12`, because `a[b]` is the same as
`*(a+b)`, which is the same as `b[a]`. In the same way
`(*(v+y)/32)[" ." ",:+" "=@#"]` picks a character from the string
`" .,:+=@#"`, which is also split into three pieces that the compiler joins
back together.

6. Unsequenced side effects

`n++[C] = t*n/12` both changes `n` and reads `n` in one expression. The C
standard does not say which happens first, so the result depends on the
compiler. GCC reads the old value of `n`, so the deobfuscated code uses
`palette[i] = seed * i / 12` with the counter increased afterwards.

7. Operators squeezed together

`D=p**a+++(1-p)**z++` is `D = p * (*a++) + (1-p) * (*z++)`: the `**` is a
multiply followed by a pointer dereference, and `+++` is a post-increment
followed by a plus. Likewise `(*J+++C[1])` is `(*J++ + C[1])`.

8. A two-dimensional loop flattened into one `for`

`for (x=-X/2,y=-Y/2;y<Y/2;++x>=X/2?x=-X/2,y++:4)` walks over every pixel
with a single loop. The step uses the `?:` operator to reset `x` and move to
the next row when the row is done, and `4` is a dummy value that does nothing.
This is the same as two nested `for` loops over `y` and `x`.

9. Hidden constants and tricky conditions

`44.0/7` is used as an approximation of 2*pi (one full turn of a circle), and
`B=11.0/630` is the speed of the palette cross-fade. On DOS, the vertical
retrace wait `for(; i>=0; i-=8)while(~inportb(0x3DA)&8^i);` runs twice: with
`i=8` it waits until bit 3 of the VGA status port is clear, and with `i=0` it
waits until that bit is set again. The `^ i` flips the test between the two
passes, so one loop does the job of two different `while` loops.

10. One function with two meanings

`F(i)` does two unrelated things: `F(0)` sets up the screen and `F(8)` shows
a frame. The value `8` is also reused as the starting value of the DOS retrace
loop. The curses version even declares it with an old-style untyped
parameter, `void F(i)`, so `i` is silently an `int`.

11. Hidden behavior through preprocessor branches

`#ifdef __DJGPP__` / `#elif defined(SDL)` / `#else` picks one of three
completely different versions of `F` (DOS VGA, SDL or curses). The curses
version also quietly shrinks `X` and `Y` to fit the terminal before the main
program uses them, and it ignores the palette, so the colors only exist on DOS
and SDL.

12. Type punning a byte array

In the SDL version `SDL_SetColors(s,h,0,256)` passes a plain
`unsigned char` array where an array of `SDL_Color` structures is expected.
It works because each `SDL_Color` is four bytes (red, green, blue, unused),
and the palette is filled with that same layout, leaving every fourth byte
unused.
