# 2024/straadt

Difficulty: Hard

1. Single letter names

Every global and function has a one or two letter name: `T` is the step
counter, `Q`/`dp` give the position inside the current beat, `qb` is a square
wave, `G` hands out filter memory, `Y` mixes a voice into the output, `L`/`W`
write 16/32 bit numbers and `Z` writes text. The arrays `q0`, `q1` and `q2`
hold the bass notes, the note tables and the strings. You have to read each
function to learn what it is for.

2. Swapped names

The author says `cm` and `ap` stand for "allpass" and "comb", but it is the
other way round in the code. `ap` is really the comb filter (a delay line with
a low-pass in the feedback) and `cm` is the all-pass filter. The names point
you in the wrong direction.

3. "Encrypted" strings

`Z()` writes each character minus one, so `"SJGG"` becomes `RIFF`,
`"XBWFgnu!"` becomes `WAVEfmt ` (this is the "fmt"=>"gnu" joke), `"ebub"`
becomes `data`, `"opx!fokpz!"` becomes `now enjoy ` and `"qsph/xbw"` becomes
`prog.wav`. The last two are written right after the sound data in the big
buffer `M`, so the file name and the final message are both stored in the same
memory as the wav file. `puts(pa)` prints `now enjoy prog.wav` because the two
strings sit next to each other with no terminator in between.

4. Macros that hide math and code

`S`, `O` and `H` are just `sin`, `pow` and `tanh`, which makes the formulas
hard to recognize. `A(n)` hides a `for` loop over `j`. `C(b)` contains the
whole body that the two delay filters share: it grabs memory with `G`, reads
the current position stored as a double in the last slot, runs the code `b`,
moves the position forward and returns the result.

5. Header written with tiny helpers and shifts

The wav header is built from calls like `W(1<<18)` (sample rate 262144),
`W(4<<18)` (bytes per second), `L(1)` (PCM), `L(2)` (stereo). `L` writes
`w>>3>>5` instead of `w>>8`, and `W` writes `l>>13>>3` instead of `l>>16`, so
the byte order is harder to spot.

6. Loop exit hidden next to a switch

The main loop is `for(;;)` with no condition. The only way out is
`if (++I>N) break;` which is written right after the closing brace of the
hi-hat `switch`, so it looks like it belongs to the switch. It also increments
the sample counter in the middle of a sample, so the kick drum and hi-hat tone
see the old sample number while every voice after it sees the new one.

7. Fall-through default label

`default : case 1 :` puts the `default` label on the same line as `case 1`, so
every section other than 0 and 2 uses the case 1 hi-hat rhythm.

8. Numbers written in other forms

`B=17e3` is an `int` set from a floating point literal (17000), and
`N=(B*5)<<8` is the song length. The hi-hat and bass rhythms are hidden in hex
numbers (`0xeceddaca`, `0xfeedfaad`) where each bit is one beat, and the bass
notes are nibbles of `0x0ce97522` and `0xc02972e5`.

9. Hidden "memory corruption"

All filters take their memory from the big array `D` in the order they are
called each sample. The all-pass lengths in the reverb depend on `T(8)`, so
when the song moves to a new section the later filters start using memory that
used to belong to other filters, including the slot that stores the read
position. This garbled state is used on purpose as a sound effect.

10. Misleading formatting and a fake brace

The code is laid out in odd shapes: one-line functions are padded with spaces
so the `;}` lines up on the right, the header code is broken into tiny pieces
across many lines, and the end of `main` is spread out so that keywords like
`unsigned` and `signed` stand alone. A `/*{*/` comment is placed inside the
progress code to confuse the bracket matching of editors like vim.

11. Declarations in strange orders

`double static`, `int static` and `void static` put `static` after the type,
the `#include` lines are placed in the middle of the file after the helper
functions, and variables like `P` and `M` are declared at the end of the `q2`
string array declaration.
`main` also sets `px=P=M` inside its declaration list.
