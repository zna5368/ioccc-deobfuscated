# 2011/blakely

Difficulty: Hard

1. Source code laid out as a Life pattern

All the code is placed so that its characters, treated as live cells,
evolve into the digits of pi after four generations. Tokens are scattered
across the page with large gaps of spaces, and whole statements are split
over many lines (`if (x - 1== i )` and `j\n=    -1`). Some code is written in
an odd way only to fit the shape, for example `f[i]-=- 3` instead of
`f[i] += 3`. The rewrite uses a normal layout.

2. Keywords and headers hidden in the Makefile

The source never says `char`, `int`, `return`, `memset` or `sizeof`. The
Makefile passes `-Do=char -Dn=return -Dd=int -Dz=memset -Dk=sizeof`, so
`d b,x,h;` declares ints, `n 1;` returns 1 and `z(f,2,x*k(o))` is
`memset(f, 2, x * sizeof(char))`. The `#include` lines are also given with
`-include stdio.h` and friends instead of in the file. Short names were
needed because long words did not fit in the gaps of the pattern.

3. Index and array swapped (`2[a]` syntax)

In C `a[i]` means `*(a + i)`, which is the same as `i[a]`. The code uses this
to make neighbour lookups look strange: `(-b-1)[e]` is `e[-b-1]` and
`b[e+1]` is `e[b+1]`. It goes further with `b[i + f + 1 - b]`, which looks
like an index into the integer `b` but is `*(f + i + 1)` because the two
`b`s cancel. `(a ? i+1 : 0)[f] = 2` picks either `f[i+1]` or `f[0]` to
write. The rewrite uses plain `grid[i]` indexing.

4. Conditional assignment with XOR

`q[x-1] ^= (q[x-1]==32) ? 0 : q[x-1] ^ 35;` uses the fact that `v ^ (v ^ w)`
is `w`. XOR with 0 keeps the value and XOR with `v ^ 35` turns it into 35.
So any non-space character becomes `#`. The same trick sets the trial cells:
`b[i+f] ^= !t ? 0 : b[i+f] ^ ...` sets `f[i+b]` only when `t` is true. The
rewrite uses ordinary `if` statements.

5. Calculated character codes

Character constants are worked out with arithmetic. The dashes and newline
of the header line are `5*(b==i?2:9)`, giving 10 (`'\n'`) or 45 (`'-'`).
Dead and alive cells are `32 + 3/7`, `32 + 3/8` (both 32, a space) and
`32 + 3/1` (35, `#`), so the Life rules pick a divisor instead of a
character. A trial cell is `(10 + bit) * 3 + 2`, which is 32 or 35. The
"unknown" value 2 becomes a space with `f[i] <<= 4` (2 * 16 = 32) and then
`#` with `f[i] -= -3`. The rewrite uses `DEAD`, `ALIVE` and `UNKNOWN` names.

6. Parity and bit tests instead of comparisons

A cell counts as a live neighbour when `value % 2` is 1, because 35 is odd
and 32 and 2 are even. Whether the wanted cell is alive is tested with
`(q[i] & 2) == 2`, and "2 or 3 neighbours" is `(u&2)+(u&4) != 2`. The test
`(f[i]+q[i]) % 2 == 1` means "the old and new cell differ". The three
reject conditions were combined into one call of the Life rule:
`lives_next(old_alive, count) != wanted_alive`.

7. Needless recursion

The printing function `w(i)` prints one character and calls itself with
`i+1`, and the Life step `m(i)` computes cell `i+1` by calling itself before
it works out cell `i`. Both are just loops over the grid and have been
rewritten as `for` loops. Only the backwards search, which really needs
backtracking, stays recursive.

8. Comma operator and nested ternaries

`m()` returns `(i==x-1) ? 0 : (f[i+1]=m(i+1)), 32 + 3/(...)`. The comma
means the recursive call is done first and thrown away, and only the part
after the comma is the result. The line reading input,
`x -= (q[x-1]==10) ? b+=1,1 : 0;`, counts a row and steps back over the
newline in one expression. The number of choices to try is a three-level
ternary, `i-i/b+2==b ? 1 : i>2*b ? i%b!=1 ? 2 : 2*1<<1 : 8`, rewritten as
the function `choice_count()`.

9. `goto` loop

The backwards search loops over its choices with a label `l:` at the top,
`goto l` to try the next choice and `goto c` to leave. This is a `for` loop
with `continue`, and is written that way in the rewrite.

10. Variables doing more than one job

`x` is one more than the number of cells (the extra slot is where EOF was
stored), so many tests use `x - 1`. `b` is both the number of lines and the
width of the grid, since the grid is square. The global `h` is set as a
side effect inside `p()` and then read in the same expression as
`p(q,i) > 3 || h < 2`. The rewrite names these `grid_end`, `size` and
`neighbors`.

11. Single letter names

Every variable and function has a one letter name: `q` and `f` are the
current and next grids, `w` prints, `p` counts neighbours, `m` does one
Life step and `y` does the backwards search. They were renamed to describe
what they hold or do.

12. Reading before the start of an array

The neighbour count for the top row reads `q[-b-1]` and similar, which is
before the start of `q`. It works because `f` happens to be placed just
before `q` in memory and its end is never written, so those cells read as
dead. The rewrite puts both grids in one buffer so that this is always true.
