# 1998/bas2

Difficulty: hard

1. Character recognition by bit matrix

The string `"t8B~pq\`"` is not text at all. It is a 7x7 grid of bits. Row `j`
of the grid is the character `"t8B~pq\`"[j]`, and column `k` is made by taking
bit `k` of every row. Read column by column, the grid spells out the seven
special characters the program cares about: bit 0 is space, bit 1 is form feed,
bit 2 is tab, bit 3 is newline, bit 4 is `;`, bit 5 is `{` and bit 6 is `}`.
For every input character the program starts with `O = -1` (all bits on). Then
for each bit `j` of the character it does `O &= bit ? I[j] : ~I[j]`, so a
column stays set only if every bit matches. In the end `O` holds exactly one
set bit naming the special character that was read, or 0 for any other
character. simple.c replaces this with plain `==` comparisons in
`is_whitespace()` and `is_semi_or_brace()`.

2. Comparing bit values instead of naming classes

The bits from technique 1 are sorted so that a simple size test tells the
classes apart. `O > 8` (or `O > 9`) is true only for bits 4-6 (`;`, `{`, `}`),
a nonzero `O` of 8 or less means whitespace, and `O == 0` means any other
character. So `i += O>8 ? o : O ? 0 : o+1` is really a three-way "if" on the
character class, written as nested `?:` operators.

3. Recursive main() instead of loops

There are no loops. `main()` calls itself once for every bit of every input
character. The argument `l` holds the remaining bits of the character
(`main(l/2)` shifts one bit away each time). The pointer `I` walks through
the pattern string at the same time. When it reaches the string's ending
`'\0'`, that bit is done, so the next character is read with `getchar()` and
`I` is reset to the start of the pattern. The first call uses `argc` (which is
1) as `l`. simple.c uses a normal `while` loop over `getchar()`.

4. Delayed counting with a "previous character" flag

A character's score is not added when it is read. It is added one step later,
when the next character arrives (or at end of file). This is because a `;`,
`{` or `}` only counts if the character after it is not whitespace. The
variable `o` remembers whether the previous character was one of those three.
Because the first call always adds a bogus 1 (when `O` and `o` both start at
0), the final `printf("%d\n", --i)` subtracts it again. simple.c adds the
score straight away and keeps the flag in `pending_semi_brace`.

5. `||` and `?:` instead of if statements

`*I++ || (...)` runs the code in parentheses only when the pattern pointer has
reached the `'\0'` at its end, which is a hidden `if`. The comma operator
packs five statements (read a character, update the count, set `o`, reset `O`,
reset `I`) into one expression, and the last part (`l > 0`) decides between
recursing and printing the result. A NUL byte gives `l == 0` and an end of file
gives `l == -2`, so both stop the program. That is why the program cannot
handle files that contain a NUL byte.

6. Meaningless names and useless layout

The variables `O`, `o`, `I`, `i` and `l` look almost the same, so it is hard
to tell them apart. The whole program is crammed into three lines. simple.c
uses descriptive names (`count`, `pending_semi_brace`) and normal formatting.
