# 2020/kurdyukov4

Difficulty: Hard

1. Macros that split statements apart

`#define R (z=a=(s*=S)>>31)*n>>32;for(` ends one statement and opens the
header of a `for` loop, so `p=R;x--;...)` becomes the random start position
followed by the main output loop. Because the macro holds half of a loop, you
cannot tell where loops begin by reading the source. `K(x)` likewise closes the
`fopen(...)` call and adds `||fseek(F,0,SEEK_x)||`, gluing file checks together.

2. Macro with a missing opening parenthesis

`H(b,x)` expands to `memset b{...}`. The caller passes `b` as `(h,-1,4*q--);`
with its own parentheses, so the macro is really a `memset` call followed by a
block that hashes the context and walks the hash table. The block is reused
both to fill the table and to look up matches, with different loop bodies
coming after the macro.

3. Macro definitions placed in the middle of code

`#define H` is written in the middle of the `if` condition that checks the
arguments, and it uses a backslash to continue onto the next line. This breaks
up the expression so it is hard to read the condition as one piece.

4. Long chain of `||` conditions instead of `if` statements

All argument and file checks (`x>>20`, `(y-1)>>7`, `fopen`, `fseek`, `ftell`,
`malloc`, `fread`, `n>>29`) are one big `||` expression ending in `break`.
Each part only runs if the earlier parts were false, so it works like a list of
"if this fails, stop" checks.

5. Range checks with bit shifts

`x>>20` is non-zero when `x` is negative or at least 2^20, and `(y-1)>>7` is
non-zero when `y` is outside 1..128. `n>>29` rejects files of 512 MiB or more.
Shifts replace normal `<` and `>` comparisons.

6. `while` used as an `if`

`while(c++==5){...c=0;}` runs once when `argc` is 5. Inside, `break` is used
to jump out on errors like an early return. At the end `c=0` stops the loop;
the final `return!!c` turns `c` into 0 or 1 (it is always 1 here).

7. Subtraction and XOR instead of `!=`

Tests like `'_'-(a=m[i++])`, `m[i++]-'>'`, `p-l?p:0`, `fread(...)-n` and
`(i=h[a&q])^-1` are true when the two values differ. They are ways of
writing `!=` that look like arithmetic.

8. Computed constants

`for(;S*5>S;)S*=5;` builds the random multiplier 5^27 by multiplying until it
overflows. `for(;l*3>q;)q+=q;` finds the hash table size (a power of two at
least 3 times the text length). `8<<8` is 2048, the byte size of `t[512]`.
The hash multiplier `79764919` is the CRC-32 polynomial `0x04C11DB7` written in
decimal.

9. Packed array instead of two arrays

`t[512]` holds, for each byte value `c`, a count at `t[2*c]` and the last
position seen at `t[2*c+1]`. The code writes `k=m[i+y]*2,t[k++]++,...,t[k]=i`
so the same index `k` moves from one field to the other.

10. Variable reuse and short names

Single letter names (`k`, `l`, `p`, `i`, `x`, `y`, `n`, `q`, `a`) are reused
for different jobs. `a` is a character in the filter loop, a hash in the
lookup and a random number in `R`. `n` is the file size, then the text length,
then the number of matches. `q` is the argument index 1, then the table size,
then the table mask after `q--`.

11. Hidden assignments inside expressions

Values are assigned in the middle of conditions and arguments, such as
`fopen(V[q=1],...)`, `y>(n=ftell(F))`, `memset(t,n=0,...)` and
`k-(a=' ')`, so it is easy to miss where a variable gets its value.

12. Loop work placed in the `for` increment

`for(k=0;k<l;h[a&q]=k++)` stores the table entry in the increment part of the
loop, and the main loop `for(;x--;p=p-l?p:0)` wraps the position around there
too. Empty statements `;;` and empty loop bodies such as
`for(a=' ';i<n&&m[i++]-'>';);` hide what is doing the real work.

13. Layout as a block of text

The code is formatted as a rectangle with statements and even identifiers
(`fte\ ll`) split across lines, so the structure of loops and blocks is not
visible from the indentation.
