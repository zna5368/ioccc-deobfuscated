# 2025/tompng

Difficulty: Hard

1. Source code shaped like a picture

The code is laid out in a large ASCII art shape, so lines break in odd
places, like `a=strtol` at the end of one line and `(*A,&B,0` at the
start of the next, and tokens are padded with runs of spaces such as
`V [   k ]`. The compiler ignores this whitespace, but a person cannot read
one statement at a time. simple.c uses one statement per line with normal
indenting.

2. Short macros for common words

`#define D double`, `#define r rand()` and `#define R (D)r/RAND_MAX` turn
the most used words into single letters. Every `R` is a hidden call to
`rand()`, so a line like `c+=R` changes the random number sequence even
though it looks like a plain variable. simple.c writes `double` and calls
`rand()` and a `random_unit()` function directly.

3. A macro that unrolls a loop

`E(i)` updates one resonator (two numbers `s[i]` and `s[i+1]` that are
rotated and shrunk each sample) and the filter `F` is just `E(0)E(2)E(4)`.
The macro uses `f[i/2]` to choose the decay, so the reader has to work out
that the three copies use `e`, `e*e` and `e*e*e`. simple.c writes this as
a three step `for` loop.

4. Single letter names and reused globals

Almost every variable is one letter: `I` and `V` are the source sounds and
their lengths, `J` and `W` are the layered sounds and lengths, `K` holds
18 sets of filter state, `O` is the loudness of each note, `Q`, `S`, `T`
are noise values and `U` holds both the play position (`U[0..2]`) and the
chosen sound (`U[3..5]`) of the three wave voices. Locals such as `a`,
`b`, `c`, `j` and `x` are reused for several unrelated jobs inside one
loop iteration (for example `a` is first the minute length `H*60`, then
the beat length `H/4`, then the chosen note). simple.c gives each value
its own descriptive name.

5. One function used for many different jobs

`F(state, decay, x, y, input)` is the only filter in the program. Passing
`x=1, y=0` makes it a smoothing filter (used for loudness tracking, slow
random drift and note envelopes) while other values make it ring at a
pitch (used for bubble sounds and hiss). Because the purpose is decided
only by numeric arguments like `1-5e-5,1,0`, the reader must guess what
each call does. simple.c comments each call site.

6. Hidden numbers and strings

The WAV file header is an array of plain integers such as `1179011410`
and `1163280727`, which are the text `RIFF` and `WAVE` stored as little
endian numbers, and `131073` and `1<<20|4` pack two 16 bit fields each.
The four musical scales are decimal numbers like `750912`, which is hex
`0xB7540`, where every hex digit is a semitone. `v[0]-45` tests for a `-`
character (ASCII 45). `28098` and `3e5` are buffer sizes. simple.c writes
the strings with `fputs`, the scales in hex and names the constants.

7. Comma operators and nested conditionals instead of if statements

Many decisions are packed into one expression, for example
`c=a?(d=R*H/2,e=1,f=1+R/5,.3):(d=...,e=...,f=...,.5)` sets four variables
and picks a value at the same time, and
`a=a<0||(j%(m*4)<m&&!b?R<.5:R<.2)?r%5:a` relies on the `?:` operator
having lower priority than `||`. The argument loop puts all its work in
the `for` increment: `a=a&&!*B?X?(srand(a),0):(X=a*H):(v=*A,0)`. simple.c
rewrites these as `if`/`else` blocks.

8. Work hidden in for loop headers

Code that belongs in the loop body is moved into the increment part of
`for`. The layer builder updates `b=J+l, p=W+l` (the layer below) in the
outer loop increment, the wave mixer updates the length with
`V[k]=fmax(V[k],j+b+1)` there, and the output loop writes both bytes of
each sample with `fputc(j=...,w),fputc(j>>8,w)` in its increment, after
the body has computed `z`. Loops also count down with `k--` in the
condition, which makes the first index `P-1` and not `0`. simple.c moves
the work into the bodies.

9. Integer and floating point mixing

Many values are silently converted. `m=j*e` and `n=fmax(j-d,0)*f` turn a
stretched floating point position into an integer index, `fmax` is used on
integers to get a maximum, and `k/5+(h[l]>>k%5*4)%16/12.` mixes integer
division (octave) with floating division (semitone). `!d` is used as the
number 0 or 1 in `.4*x*!d`. simple.c keeps the same arithmetic but makes
the intent clear with `if` tests and named variables.

10. Unused names

The global `SEED` and the `main` parameter `_` are never used, and the
array `f[7]` holds two unrelated groups of values (three wave samples and
four slow drift values). simple.c removes `SEED` and splits `f` into
`wave[3]` and `drift[4]`.
