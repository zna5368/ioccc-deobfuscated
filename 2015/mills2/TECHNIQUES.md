# 2015/mills2

Difficulty: Hard

1. Input file used as the data structure

`e(L-1)` and `e(c=L)` look like ordinary recursion, but `L` is a position in the
file rather than a value. Most LZW decoders build a dictionary table in memory.
This one has no table. It seeks back into the compressed file to the two spots
where the encoder created an entry, and decodes those again. Nothing in the
source says this, so the reader has to work out the idea before any line makes
sense. `simple.c` calls these positions slots and explains them at the top.

2. One function doing everything

`void e(int L)` finds a code in the file, reads it, unpacks its bits, handles
the end of the file, handles the clear code, prints characters and recurses.
`simple.c` splits this into `code_width`, `bit_offset`, `read_code` and
`print_slot`.

3. One parameter with two meanings

`L` starts as a slot number. `pread(3,&L,3,...)` then overwrites it with bytes
from the file, and `L>>=7&o` and `L%=1<<l` trim those bytes down to a code. After
that line `L` is a code value, not a slot. `simple.c` uses `slot`, `window` and
`code`.

4. Nested ternary operators as control flow

`L>>8?256-L?e(L-1),c||(e(c=L),c=0):(O+=...,I=L):putchar(L);` is a whole
three-way decision written as a single expression. `L>>8` is true for codes of
256 and above, and `256-L` is true for anything except 256. So the branches are
dictionary entry, clear code and literal character. `simple.c` uses `if`,
`else if` and `else`.

5. Short-circuit `||` used as an `if`

`c||(e(c=L),c=0)` only runs the right side when `c` is zero, because `||` stops
as soon as the left side is true. The value of the expression is thrown away. It
really means "if `c` is not set, set it, make the call, and clear it again".
`simple.c` writes this with `if (!first_char_only)`.

6. Comma operator joining statements

`e(L-1),c||...` and `O+=...,I=L` run two statements in a place where only one
expression is allowed, which is what lets the ternaries in technique 4 work.
`simple.c` puts each one on its own line.

7. Global flag that silently changes what the recursion does

`c` is not a character. While it is nonzero, every call skips its suffix step,
so the recursion falls straight down the prefix chain and prints only the first
character of the string. This is how `e` gets "first character of this entry"
out of the same code that prints whole strings. The flag is set to `L` just
because `L` is known to be nonzero there. `simple.c` names it `first_char_only`
and only ever sets it to 1.

8. Single-letter names that look alike

`l`, `I`, `o`, `O`, `L` and `c` are easy to confuse with each other and with the
digits 1 and 0. `I`, `l`, `L` and `O` all appear within the same few lines, and
`main` declares its own `l` and `o` that hide the globals. `main`'s `l` is
really `argc` and its `o` is really `argv`. In `simple.c` they are `next_slot`,
`width`, `slot`, `segment_start`, `argc` and `argv`.

9. Globals used as temporaries

`l` and `o` are global but only hold values for the length of one call. Every
recursive call overwrites them, which only works because they are never read
after a recursive call returns. `simple.c` makes them local variables.

10. Loop with its work hidden in the header

`for(l=8;L>>++l&&16>l;o+=l<<l-1);` has an empty body. The increment of `l` is
buried inside the condition, and the loop computes two things at once: the code
width of the slot, and the total bits taken by all the narrower slots. Each width
`l` holds `1<<(l-1)` slots of `l` bits, which is where `l<<l-1` comes from. The
next line, `o+=l*L-(l<<l-1)`, adds the rest. `simple.c` has one function for each
result and a comment on the formula.

11. Unexplained magic numbers

`O=3` is the size of the file header. `I=256` is the first slot, and the same
256 is also the clear code and the number of literal characters. `8`, `9` and
`16` are the group size and the code widths. The `open` mode is written as `0`.
`simple.c` gives each of these a name in an `enum`.

12. Hardcoded file descriptor

`open(1[o],0)` throws away the value that it returns, and then `pread(3,...)`
uses the number 3. Programs start with descriptors 0, 1 and 2 already open, so
the next `open` normally gets 3. `simple.c` keeps the returned value in
`input_file`.

13. Reversed array index

`1[o]` is `o[1]`, because `a[b]` is defined as `*(a+b)` and addition works in
either order. With `o` being `argv`, this is the first command line argument.

14. Bit tricks in place of plain arithmetic

`-I&7` is how many more codes are needed to reach the next multiple of 8, and
`7&o` is `o%8`, which is the position of a code inside its first byte. `>>3`
divides by 8 to turn bits into bytes. Operator precedence adds to this, because
`(-I&7)*l+o+l>>3` adds first and shifts last, and `l<<l-1` subtracts first and
shifts last. `simple.c` adds parentheses and uses `/` and `%` where they read
better.

15. Reading into the bytes of an `int`

`pread(3,&L,3,...)` reads 3 bytes straight into the memory of a 4 byte `int`. On
a little-endian machine the first byte of the file lands in the lowest bits of
the number, so this works as a quick way to pull in a code that crosses byte
boundaries. A code is at most 16 bits and can start at any of 8 bit positions, so
3 bytes is always enough. A read of fewer than 2 bytes means only padding is
left, and that is the only way the program ever ends. `simple.c` keeps the read
but names the sizes and explains it.

16. Infinite loop with a hidden exit

`for(open(1[o],0);;e(I++));` has setup code in the first part of the `for`, no
condition and no body. It looks as if it never stops, because the real exit is
the `exit(0)` several calls deep inside `e`. `simple.c` moves `open` out of the
loop and says with a comment where the program ends.

17. ASCII art layout and decoy code

The source is arranged as ASCII art, which splits expressions across lines in
odd places such as `o+=l` / `<<l-` / `1)`. The pieces `{ ; }`,
`/*    */` and `/*    ////      */` do nothing and are there to fill out the
shape, and the sad face `:( O` is really the `:` of a ternary, an opening
parenthesis and the variable `O`. `simple.c` has normal formatting and drops the
filler.
