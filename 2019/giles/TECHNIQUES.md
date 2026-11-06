# 2019/giles

Difficulty: Hard

1. Code shaped as ASCII art

The source is laid out as three curved arcs that look like sound waves.
Statements are split in the middle of names and numbers, for example
`s[t]` becomes `s[` at the end of one arc and `t]` at the start of the
next. The picture draws your eye away from the code, and lines don't match
statements, so it's hard to tell where one statement ends.

2. Short macros that hide simple operations

`#define k 16` and `#define i 24` replace plain numbers, so `f[k+A]` really
means `f[16+A]`. `q(a,b)` multiplies two entries of `w[]`. `u()` rounds a
fraction to a percentage, `l()` clips a float to the 16-bit sample range,
`D()` calls `setvbuf`, and `g()` prints an error and returns 1. The macro
names are one letter long and tell you nothing about what they do.

3. Clamping done with a double negation

`j(j,J)` expands to `w[j]=w[j]>J?-(J):-w[j];`, which limits a value and
flips its sign. Calling it twice flips the sign back. `j(1,1)` followed by
`j(1,0)` ends up clamping the dry mix to the range 0 to 1, and
`j(0,6e5f)` with `j(0,-4e3f)` clamps the room size to 4000..600000 samples.
The two calls in each pair are far apart and mixed in with file handling,
so you have to trace both to see that it's just a `clamp()`.

4. Macro with unbalanced brackets

`C(A,B)` contains the whole all-pass filter step, but its body has more
`[` than `]`. It is always called like `C(v],+10)`, where the first argument
brings the missing `]`. So `t[A[d[A]` becomes `t[v][d[v]]`. The second
argument is pasted after a number, so `w[2 B]` becomes `w[2 +10]`, which is
`w[12]`. This means the same macro can work on either the left (`w[10..14]`)
or right (`w[15..19]`) group of temporary values.

5. XOR-encrypted strings

All of the strings ("WAVE", "data", "fmt ", the file modes, file names,
error messages and the `printf` format) are joined into one string literal.
At runtime it's read as an array of 32-bit integers, and integer `s` is
XORed with `16843009*(s+1)`. Because `16843009` is `0x01010101`, every byte
in that integer gets XORed with `s+1`. The result goes into `O[]`. Each
string is then reached by a byte offset from `y`, for example `y+6` is
"Bad input file" and `y+99` is the `printf` format.

6. One array used for many different things

`int32_t f[88]` holds the 44-byte WAV header (`f[0]`..`f[10]`), the channel
count (`f[12]`), the number of output frames (`f[13]`), an end-of-file flag
(`f[14]`), the number of active delay lines (`f[15]`), the write positions
(`f[16]`..`f[39]`), the read positions (`f[40]`..`f[63]`) and the delay line
lengths (`f[64]`..`f[87]`). `float w[21]` likewise holds the parameters,
the filter constants and all of the temporary sample values. Nothing in the
code says which slot means what. In the clean version each one gets its own
named variable, and the header is a `struct`.

7. A pointer that moves while you read it

`int32_t *c = f+1` points into the header but is changed in the middle of
expressions: `*++c != *o++ || 2[--c] != 1[o] || 8[c--] != *o` moves `c`
three times in one line. To find out which header field is being checked
(here `f[2]` = "WAVE", `f[3]` = "fmt ", `f[9]` = "data") you have to track
every `++` and `--` by hand. Then you have to look up the WAV file format
to see what is at that byte offset.

8. Overlapping buffers

`K *e = (K*)(f+10)` makes a 16-bit sample buffer on top of the last two
header slots. After the header has been written out, those bytes are used
again for the audio samples. `e[1]` is the left sample and `e[f[12]]` is
the right sample (or the left one again for mono files).

9. Unusual array indexing

`a[b]` is the same as `*(a+b)`, which is the same as `b[a]`. The code uses
this everywhere: `s[w]` for `w[s]`, `12[f]` for `f[12]`, `10[f+4]` for
`f[14]`, `3[b-2]` for `b[1]`, and `*X` for `X[0]`.

10. Assignments and comma operators hidden inside expressions

An assignment gives back the value it stored, so it can be put inside
another expression. `f[1]=20+(10[f]=f[13]<<2)+k;` sets the data size and
the RIFF size at the same time. `w[0]*=(12[f]=4[++c]>>k,6[f]);` saves the
channel count into `f[12]`, moves `c`, and then multiplies by the sample
rate, all in one statement. `s[z]=(s[d]=0);` clears two arrays at once.

11. Roundabout ways to set values

`while(++f[15]<12);` is an empty loop that raises the number of active delay
lines to at least 12. `for(s=14[f]=0;...)` clears the end-of-file flag
inside the loop header. `e[f[12]=1]=0;` switches to mono and clears the
sample in one go when the input runs out.

12. Magic constants

The reverb constants are bare numbers in `w[]`: `.77f` is the all-pass
feedback, `.965926f` and `.258819f` are the cosine and sine of 15 degrees
(used to rotate the left/right channels into each other), `.64731f` is how
much shorter each pair of delay lines gets, and `.9f` makes the right line
90% as long as the left. New header values are worked out with arithmetic,
for example `5+4*(a>>=1)` with `a=65535` gives `0x00020001` (PCM format,
2 channels). That same line also halves `a` to 32767 so it can be used
later as the sample clipping limit.

13. Hidden extra parameter

The option loop runs `s` from 0 to 4, so a seventh argument can overwrite
`w[4]`, the all-pass feedback. The README never mentions it.

14. Decoy comment

A `/* ... */` comment that runs across several arcs holds fake code such as
`printf("Hello, world!");`, and the last line ends with a `//` comment full
of code-looking junk. It looks like real code, but the compiler ignores all
of it.

15. Single-letter names

Every variable is one letter: `t` is the delay line buffers, `z` the low-pass
filter state, `X` the delay lengths, `n` and `h` the output and input files,
`m` the cleanup function. The names give no clue what any of them are for.
