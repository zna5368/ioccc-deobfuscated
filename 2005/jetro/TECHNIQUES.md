# 2005/jetro

Difficulty: Medium

1. Song data built by macros from stringized tokens

The song is never written out as text. Macros such as `S1(a,b,c)` turn their
arguments into strings with `#a` and glue them to dots, so `S1(J,H,M)` becomes
`"J..H..M."`. Other macros repeat pieces: `X2`/`X4` copy their argument two or
four times, `S4(..)` makes eight dots and `SE` makes 128 dots of silence. The
compiler joins all these small strings into the long arrays `l` (melody), `b`
(bass) and `r[3]` (three chord parts). You have to expand the macros by hand
to see the notes.

2. Notes stored as letters

Each character of a song string is one step of the music. A `.` (written as
`46` in the code) is silence. A letter is a pitch: `n=x-(x>=97?121:65)+a`
turns `'A'` into 0, `'B'` into 1 and so on, while lowercase letters count from
`'y'`, so `'a'` is 24 semitones below `'A'`. The pitch becomes a frequency with
`pow(2,n/12.)`, the standard "twelve notes per octave" formula.

3. Macros that hide whole statements

The sound code in `AA` is mostly macro calls such as
`IP(u,NQ(u,12,1)m=FQ(1.01f);...,12288,0.2f,;)`. `IP` expands to an `if`, a
filter and a store into the buffer; `NQ` decodes a note, `FQ` makes a sine
wave, `IZ` keeps only the peaks of a wave and `IS` clamps a value. Some
arguments are whole lists of statements, and some are left empty, so the
real control flow only appears after the preprocessor runs.

4. Macros that rename C keywords and types

`B` is `int`, `C` is `double`, `S` is `static` and `A` is a `typedef` for
`float`. This makes declarations like `S B D,R,U,p=0;` hard to read.

5. Pasting numbers together

`#define EF_(x) F[w][1638##x+M` uses `##` to glue a digit onto `1638`, so
`EF_(4)` gives the index `16384` and `EF_(3)` gives `16383`. The macro also
leaves its `[` bracket open; the closing `]` comes from the code that uses it.

6. A mirrored ring buffer for the echo

`F[5][0100000]` (the octal number is 32768) stores the last 16384 samples of
each of the five voices twice, once at `M` and again at `M+16384`. Reading
`F[w][16384+M-12288]` then gives the sample from 12288 steps ago, and
`F[w][16383+M]` gives the previous sample, without ever wrapping the index.
These are used to add an echo and to smooth the sound.

7. Octal and odd number spellings

Masks and sizes are written as `037777`, `017777` and `0100000` (octal for
16383, 8191 and 32768), `0x2000` and `8192e0f` for 8192, and `6e-1`, `5e-1f`
and `1e-2` instead of plain decimals. `SDL_Init(16)` hides the name
`SDL_INIT_AUDIO`.

8. Index tricks

`(s%R)[l]` means `l[s%R]` and `a[r]` means `r[a]`, because in C `x[y]` is the
same as `*(x+y)`. `(!!c&24)[r]` picks `r[0]`: `!!c` is 1, and `1&24` is 0.

9. Strange ways to step through the buffer

The output pointer is moved with `_++,++_` (two bytes per 16 bit sample) and
the length is reduced with `--L,L--`. The pointer is named `_`, and the
callback is named `AA`.

10. Shadowed and self-assigned variables

The callback's `u` parameter is assigned to itself (`u=u;`) only to avoid an
unused warning, and then a new `int u` inside the loop hides it and holds the
melody note. `main` does the same with `c=c,a=a`.

11. Unused code and dead variables

The macro `I(a,b,c,d,f,g)` (a Lorenz attractor step) and the global `T` are
never really used; they are only there to distract.
