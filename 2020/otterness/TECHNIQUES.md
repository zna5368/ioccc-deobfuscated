# 2020/otterness

Difficulty: Hard

1. No literals at all

The source has no number, character or string literals. Every constant is
built from the global variables, starting from `k = !!stdin` in `main()`, which
is always 1. From there `n = k + k + k` is 3, `l = k << n` is 8, and so on.
Even the letters of `"MTrk"` are computed: `*b = l << n | q` gives 77 (`'M'`),
and `p = ~k++; p >>= k--;` gives `0x7f`. To find the value of any expression
you have to work out every assignment that happened before it.

2. Globals that change value all the time

The nine globals `k l m n o p q r s` are reused for different constants in
different places. `l` is 25 in `main()`, 8 when reading big-endian numbers, 7
while reading a variable-length integer, 4 while checking a status byte, and
40 while writing beats. `k` flips between 0 and 1 through `k++` and `k--`
inside expressions, so the same `k` means different numbers on lines next to
each other. Functions such as `C()` and `H()` change a global, use it, then put
the old value back (`l = s;`, `q = h; m = i;`). So a function only works if
the globals hold the expected values when it is called.

3. Values that fix themselves the first time

In `G()`, `m = m < p ? p << q : m;` turns `m` from 107 into `0xF0` (the
`u1` type cuts off the high bits of `0x7F0`) on the first call, and leaves it
alone after that. `q` is changed the same way. `J()` has a block guarded by
`if (l > o)` that runs only on the first beat and sets up the note numbers
36, 40, 42 and velocities 80, 100, 120 from the old globals.

4. Short meaningless names

The functions are named `A` to `K` and the variables `a` to `j`. `A` reads a
16-bit big-endian number, `B` writes one, `C` reads a MIDI variable-length
number, `D` counts how many bytes such a number needs, `F` writes one, `G`
skips a MIDI event, `H` measures one track, `I` finds the longest track, `J`
writes one drum beat and `K` builds the new track. Nothing in the names hints
at this.

5. Reversed array indexing

C lets you write `a[b]` as `b[a]`, since both mean `*(a + b)`. The code uses
this to make array accesses look like something else: `n[c]` is `c[3]`,
`k[t]` is `t[1]`, `r[t]` is `t[2]` and `d[c]` is `c[d]`. Other accesses are
written as pointer arithmetic, like `*(c + !!c)` for `c[1]` and
`*(l + p + c)` for `c[135]`.

6. Hidden counts and tricky comparisons

Some numbers are written so they do not look like numbers. `!!c` is 1 because
the pointer is not NULL, `!b` in `return !b;` is 0, and `fwrite(c, a, !!c,
stdout)` writes one block. `d<s>> k` in `C()` is spaced to look like a
template or a comparison chain, but it is really `d < (s >> k)`, which is
`d < 4`. `k = k ? !k : k;` is just a long way to write `k = 0`.

7. Error codes computed from globals

Every `exit()` value is a computed expression like `exit(l / n + k)` (9) or
`exit((m << k) - (n << k))` (10). One of them, `exit(!k)` in `C()`, is
supposed to be error 1 but `k` is 1 at that moment, so the program exits with
0. You have to trace the globals to know which code each failure gives.
