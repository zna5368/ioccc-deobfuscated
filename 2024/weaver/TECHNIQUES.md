# 2024/weaver

Difficulty: medium

1. Single letter names

Every variable and function has a one letter name. `A`, `B`, `C` and `D` are
the music, `E`, `G` and `H` are the pictures, `g` is the color palette, `b` is
the sound buffer, `a()` writes a voice of the song into the buffer and `h` is
the order the pictures are shown in. Many globals such as `i`, `j`, `n`, `l`,
`o` and `S` are shared between the setup code, `a()` and the animation loop,
so you have to follow where each one is reset to understand what it holds.

2. Data stored as printable characters

The notes, note lengths and pictures are strings where each character is a
small number plus an offset. In `a()`, `*n-58` and `*l-58` take away the code
of `':'` to get a note number and a length. In the picture loop `*W-58` gives
a color code and `*(++W)-56` takes away the code of `'8'` to get a run
length. Because the strings look like random punctuation (`"~\\^__a^\\Z"`),
you cannot tell they hold music and pictures until you work out the offsets.

3. Run length encoded pictures

The pictures are not stored pixel by pixel. In `r=c<16?1:c<32?2:*(++W)-56`
the value of the character decides how many spaces to draw: codes below 16
draw one space, codes 16 to 31 draw two, and higher codes read the next
character as the count. `c&=15` then keeps only the low four bits, which
pick one of the 16 colors. A frame ends at the `'~'` character, tested by
`while(*W<'~')`.

4. Colors written as decimal numbers

The palette `g[]` holds numbers like `9474192`. Written in hexadecimal this
is `0x909090`, a gray. Each number packs red, green and blue into one value
and `(g[c]>>16)&255`, `(g[c]>>8)&255` and `g[c]&255` pull the three parts
back out for the `\x1b[48;2;R;G;Bm` escape that sets a 24-bit background
color.

5. Floating point constants for integers

Sizes are written in scientific notation, such as `Z=704e3`, `(*l-58)*1e3`
and `8e3/f[...]`. These are `double` values that are quietly turned into
`int` when stored, so `704e3` is the 704000 byte sound buffer, `1e3` is 1000
samples (1/8 of a second) per length step and `8e3` is the 8000 Hz sample
rate. `usleep(Z/3)` reuses the buffer size as the delay between frames.

6. Hidden square wave synthesis

`w=*n>'z'?0:8e3/f[*n-58]` finds how many samples one wave of a note lasts,
or 0 for a rest (`'~'` is the only note character above `'z'`). The test
`j%w<w/2` is true for the first half of every wave, which makes a square
wave, and `(j>d/2)?32:63` makes the second half of each note quieter. The
table `f[i]=440*pow(2,(i-45)/12.0)` is the musical scale where note 45 is
A at 440 Hz. Calling `a()` a second time after `o=0` adds the bass voice on
top of the melody in the same buffer.

7. Packed control flow

The program is written with very few line breaks, and several statements
are stuffed into `for` headers, such as `for(j=0;j<d;j++,o++)` which moves
through the sound buffer and the note at the same time, and
`for(i=0;i<r;i++,x++)printf(x%80?" ":"\n ")` which also counts the screen
column so a new line starts every 80 spaces. `fork()` splits the program
into a child that plays the sound and then spins forever with `for(;;);`
so that it never reaches the animation loop, and a parent that draws the
frames forever.
