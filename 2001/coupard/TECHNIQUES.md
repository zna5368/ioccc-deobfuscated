# 2001/coupard

Difficulty: Hard

1. Huffman compressed data in a string

The variable `d` holds all of the speech data (the word table and the sound
samples) compressed with a Huffman code, so nothing in the string can be read
directly. Huffman coding gives common values short bit patterns and rare
values long ones. The decoder in the `k` macro walks a binary tree one bit at
a time until it reaches a leaf, which is the decoded value.

2. Base 91 encoding of bits in printable characters

The compressed bits are stored 13 at a time in pairs of characters. In
`d[(i/13)*2]*91+d[(i/13)*2+1]-3220` the two characters are treated as digits
of a base 91 number whose smallest digit is `'#'` (35), and `3220` is just
`35*91+35` folded into one constant. The mask `4096>>(i%13)` then picks out
bit `i` starting from the most significant bit.

3. Huffman tree packed into a string

The string `"C9B7351A@D-/E+F?')G>H%J#=I"` is the whole decoding tree.
Subtracting 59 from a character gives either a value of zero or more (a leaf
symbol) or a negative number whose negation is the index of the next node.
The two branches of node `l` are at `l` and `l+1`, which is why the macro
indexes with `bit?l+1:l`. The expression `l=-k*(k<0)` moves to the next node
on a negative value and resets to the root (0) on a leaf.

4. Everything packed into one `for` loop header

The decompressor in `main` has an empty body. The tree walk, the stores into
`p`, the output position update and the node update are all squeezed into
the increment part of the `for` with commas and nested `?:` operators. The
`k` macro is expanded several times in that one expression, each time
redoing the full bit lookup.

5. One buffer and one table used for several jobs

The array `p` holds the word table (the first 145 values) and then the sound
samples, which are scaled with `k<<4` and stored twice in a row to turn
4 kHz into 8 kHz. The table `o` lists where each phoneme starts in `p`, but
`o[0]` is also the size of the word table, and `h[o]` in `e` reuses the same
numbers as the lengths of the pauses between words. The silence byte is the
character `d[7]` (a `'w'`) taken from the middle of the compressed data.

6. Reversed array indexing

Expressions like `j[p]`, `p[j][o]` and `h[o]` use the fact that `a[b]` is the
same as `b[a]` in C. `p[j][o]` reads as a 2D array access but is really
`o[p[j]]`, the start of phoneme `p[j]`.

7. XOR used as "not equal"

`p[j++]^9` and `j[p]^9` are non-zero when the value is not 9, the end of
word marker, so `!(x^9)` means `x == 9`.

8. Number to words logic hidden in macros

`g(n)` picks which words to say for a number with chained `?:` operators.
Numbers 13 to 19 are said as "thirty" to "ninety" followed by a single "n"
sound (word 26), so "thirtyn" stands in for "thirteen". The value `-1` is
passed to `e` to mean "say nothing, only pause". The macro `x(n)` uses token
pasting (`tm_##n`) to build `tm_hour`, `tm_min` and `tm_sec`.

9. Short meaningless names and old style declarations

Single letters such as `d`, `p`, `o`, `e`, `g`, `k` and `x` hide their roles,
and `e(n,h)` is an old K&R style function whose parameters default to `int`
with no declared types. The globals `i` and `j` are shared by the decoder in
`main` and the player in `e`.
