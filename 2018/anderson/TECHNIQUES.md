# 2018/anderson

Difficulty: Medium

1. Shaped source code with misleading whitespace

The whole program is laid out as a solid rectangle of text, so tokens are
broken up by odd spacing (`k* 2`, `16*  n`, `e[ q]`) and even split across
lines with a backslash-newline (`pu\` followed by `tchar`). The compiler joins
`pu\`+newline+`tchar` back into `putchar` and ignores the extra spaces, but a
human reader has a hard time seeing where one expression ends.

2. All the work hidden inside `for` loop headers

Both loops have empty bodies. The outer loop's condition is a long comma
expression that clears a buffer, rotates three line pointers, and calls
`fgets`; its "increment" part prints the newline. The inner loop's increment
part computes the neighbour bits and calls `putchar`. Because the comma
operator evaluates each piece left to right and keeps only the last value,
a whole block of statements can be squeezed into a spot that normally holds
one small expression. In simple.c these became ordinary statements inside the
loop bodies.

3. Arithmetic filler that does nothing

Expressions like `a-a` (zero), `+0-0 +0`, `f +a-a` (just `f`), `16+16` (32)
and the fgets size `a+a -a+ a -a+a -a+ a- +a` (just `a`, 256) are padding.
They exist mostly to make the lines the right length for the rectangle and to
make simple values look complicated.

4. One array split into three pointers

`char b[a+a+a]` is a single 768-byte array, and `c`, `d`, `e` point at its
three 256-byte thirds. They hold the previous, current and next line. Each
pass swaps the pointers around (`f=c, c=d, d=e, e=f`) instead of copying
text, so the same memory is reused for a different line every time. simple.c
uses a 2-D array `buffers[3][LINE_SIZE]` with descriptive pointer names.

5. Sliding bit window in single-letter variables

`j, k, l` remember whether the characters below-left, below and below-right of
the current position are spaces, and `m, n, o` do the same for the line above.
Each step shifts them along (`j=k, k=l` and `m=n, n=o`) and then computes the
new right-hand value. They are combined into a 6-bit number `p` with weights
1, 2, 4, 8, 16, 32. simple.c instead checks each neighbour directly and sets
the matching bit, which is easier to follow.

6. Hidden "end of string" flags

`h` and `i` start at 1 and are updated with `i = e[q] && i`. Once a NUL byte is
seen they stay 0 forever. This stops the program from reading stale leftover
characters past the end of a shorter line in a reused buffer. It looks like
part of the arithmetic but is really a bounds check. simple.c does this with
the `is_space()` helper.

7. Character literals written as numbers

`32` is the ASCII code for a space and `10` (written `+10`) is the newline
character. simple.c uses `' '` and `'\n'`.

8. Lookup table built from string concatenation and indexed in place

The characters to print come from a 64-character string literal made of many
pieces (`" ......"`, `""`, `"|||"`, ...). C glues adjacent string literals
together, so the odd break points and the empty string `""` just hide what the
table looks like. It is indexed right away (`"..."[index]`), and lots of
backslashes (`\\\\\\\\` is four `\` characters) make it harder to count
positions. simple.c lays out the table as eight rows of eight characters
with a comment explaining what each row and bit means.

9. Decoy comment inside the string table

The comment `/*\  (  |||  )  |/|/ / */` sits in the middle of the string
pieces and looks like it might be part of the data, but the compiler throws
it away. The trailing comment `/* typographic tributaries */` is the hint the
author mentions: the program draws "rivers" of spaces in text.

10. Variable length array from a non-constant global

`int a = 256;` is an ordinary variable, so `char b[a+a+a]` is a variable
length array even though its size never changes. simple.c uses a `#define`
constant instead.
