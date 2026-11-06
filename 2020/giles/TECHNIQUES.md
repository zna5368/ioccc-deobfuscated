# 2020/giles

Difficulty: Hard

1. Short, misleading and reused identifiers

Nearly every name is one or two letters and many are used for more than one
thing. `aa` is a `double` holding the input sample, then the energy of a
resonator, and also a macro that prints an error and returns. `memcpy`,
`fread` and `int_16t` are `goto` labels as well as (or look like) library
names. `m` is `argc`, then a flag for "encode mode", and also a macro that sets
`z`. `e2` is the fade-in volume in one branch, the previous sample in the
other, and a macro that calls `fread`. The reader has to work out from context
which meaning applies on each line.

2. Macros that hide statements and loops

`f(f,aa)` takes another macro and a block of code, and expands them once for
each of the eight resonators (`f(2)aa f(10)aa ... f(0)aa`). This unrolls a loop
by hand so there is no `for` to read. `t(t)` is one step of the resonator
filter, `n(n)` spreads one input sample over the resampling buffer, and `j` is
an entire `if` block that picks and prints the most common key. Using `j` right
after `else` makes `else j w=r;` look like a single statement when it is
really `else if (w) {...} w = r;`.

3. One array for unrelated data, accessed as `17[l+z]`

The array `l` holds the resonator states (`l[0..15]`), the sine/cosine
coefficients (`l[16..31]`) and the per-key hit counters (`l[32..47]`). The
array `d` likewise mixes the best energy per group, a resampling buffer, the
damping factor and the peak loudness. Accesses are written backwards using the
rule `a[b] == b[a]`, so `l[z+17]` becomes `17[l+z]`, and the macro `a` stands
for `[l+z]` so the code reads `17 a`, `0 a`, `1 a`.

4. One buffer used for many things

The `char k[173]` array is used as the key lookup table (`k[0..80]`), the
bytes of the current input sample (`k+81`), the template WAV header that is
written in encode mode (`k+85`), and the WAV header read in decode mode
(`k+129`). Offsets are written as sums like `k+'+'+'R'` (= `k+125`) so it is
hard to see which part of which header is being touched.

5. Character literals as numbers

Numbers are written as characters: `','` is 44 (the WAV header size), `'Q'`
is 81 (the key table size), `'C'+'='` is 128 (the size of 16 doubles),
`'0'` is 48 (the size of `l`), `'@'` is 64, and `'$'` is 36 (the extra bytes
added to the RIFF chunk size).

6. All strings merged into one literal with a fake prefix

The error messages, the file name and the `fopen` mode are stored in one
string that starts with an unused sentence about DTMF. The pointer is moved
past the sentence with `+111`, and then each piece is found by offset:
`t+18` is "Read error", `t+'A'-'$'` is "Cannot read dtmf.bin", `t+')'` is
"dtmf.bin" and `t+'2'` is "rb". `puts("0123456789"+10)` points to the end of
the string so it just prints a newline.

7. Program data hidden in an external file

The WAV header template, the filter coefficients (sine and cosine of each DTMF
frequency) and the key table are read from `dtmf.bin`, so the source shows no
frequencies or digits at all. The key table is ordered by column then row and
repeats each digit's letters (e.g. `g` and `h` map to the same tone as `4`) so
that `strchr` returning an index gives the row (`index % 4`) and column
(`(index / 4) % 4`).

8. Hidden mode switches and tricks in initializers

`d[6]=--m?1:0.994` decrements `argc` and at the same time picks the resonator
damping (1 means a pure tone generator, 0.994 means a frequency detector).
The `fread` calls for the data file are chained with `||` inside the
initializer of `p`, and `h` is assigned inside the call to `setvbuf`. The
`#define X` macro is either empty or adds a Windows `_setmode` call to the end
of a statement.

9. Same code for generating and detecting tones

The resonator macro `t` is used both to create sine waves (by putting a single
non-zero value into the filter with no damping) and to measure how much of a
frequency is in the input (with damping). Because the same lines do both jobs,
it is not obvious what either branch is doing.

10. Clever memory tricks

`memcpy(&r,k+v+'O',2)` takes the top two bytes of a 2, 3 or 4 byte sample so
any sample size becomes 16-bit. `memmove(d+2,d+3,24)` shifts the resampling
buffer by one. `memcmp(k+'+'+'^',k+']',14)` checks the input WAV header against
the template header to validate it. `z=z;;` and comments like the quadratic
formula are noise that does nothing.

11. Source shaped like a telephone handset

The code is laid out in the shape of a phone, with identifiers such as
`memcpy` and `strchr` split across lines with backslash-newline, which makes
the statements hard to follow.
