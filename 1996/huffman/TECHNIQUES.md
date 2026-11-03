# 1996/huffman

Difficulty: Medium

1. Keyword macros

`#define x char`, `#define r if`, `#define y while`, `#define w gets`,
`#define v putchar` and others replace every keyword and library call with a
single letter. To read the code you have to keep swapping the letters back in
your head. Running the preprocessor shows the real keywords.

2. Misleading name and hints

The entry is called "huffman", and the judges say "consider who won". This
makes you think of Huffman compression, but the program has nothing to do
with it. The author's remarks about a "tactile monitor" and a "blind person"
are the real hints: the program converts text to Braille and Braille back to
text.

3. Lookup table hidden in a macro

`#define t " A?B?K?L?CIF?MSP?E?H?O?R?DJG?NTQ?????U?V?????X???????Z????W??Y??"`
looks like random noise. It is a 64 entry table: the position of each letter is
its Braille dot pattern as a 6 bit number. The left column of dots gives bits
0-2 and the right column gives bits 3-5. For example `A` (dot 1 only) is at
position 1 and `C` (dots 1 and 4) is at position 9. Unused patterns hold `?`.

4. Reversed array indexing

C defines `a[b]` as `*(a + b)`, so `b[a]` means the same thing. The code uses
forms like `a[b]`, `a[d]`, `0[b]`, `2[a+d]`, `e[a+d]` and `*(d+a+e)`, where
the index and the array are swapped or the access is written as pointer
arithmetic. All of them are ordinary array accesses.

5. Character constants written as arithmetic

Characters are never written as `'*'`, `' '` or `'\n'`. Instead the code uses
`6*7` (42, `'*'`), `4*8` (32, space), `5*2` (10, newline), `7*5` (35, `'#'`),
`6*8` (48, `'0'`) and `8*12` (96, `` '`' ``). The buffer offsets `8*5*3` and
`5*6*8` are 120 and 240, which are the start positions of the second and third
Braille rows in the line buffer.

6. Comparisons written with subtraction and `!!`

`r(0[b]-7*5)` means "if the first character is not `#`".
`y((*c-6*7)**c)` multiplies two tests together. The product is zero when the
character is a `*` or when it is the end of the string. `!!(x-4*8)` turns
"x is not a space" into 0 or 1. Lowercase is turned into uppercase with
`*c += -(*c>8*12)*32`, which multiplies a true/false test by 32 instead of using
an `if`.

7. Packed Braille columns and cells

When encoding, each character's code is split into 3 bit columns with
`a%8` and `a/=8`. Each column is stored in `d[]`, followed by a 0 column as a
separator. Each output row then prints `*` for one bit (1, 2 or 4) of every
column. When decoding, the three rows are added back into one 3 bit number per
column. Then the loop `*(d+a+e) = a[d+e-1] + (d[a+e] << (3*e))` joins each run
of non-blank columns into a single number. That number is used as the index
into the lookup table.

8. Digits through the number sign

Digits are encoded by turning `1`-`9`,`0` into `A`-`J` with
`(*c+1)%(5*2)+65`, and then putting the columns `4` and `7` (the Braille
number sign) in front. When decoding, `d[a]%64 - 12*5` checks whether the cell
begins with the number sign (value 60). Then `6*8+(t[d[a]/8/8]-4)%10` turns the
letter that follows back into a digit.

9. One giant nested `main`

The whole program is one `main` that contains both the encoder and the
decoder. Which one runs depends on whether the line has a `*` in it and
whether two more lines can be read. Spaces between words are found by checking
whether `2[a+d]+3[d+a]` is zero, which means there are extra blank columns
after the cell. The ASCII art layout hides how the `if`/`else` blocks are
nested.

10. Hidden comment skipping

Lines that start with `#` produce no output at all. This lets the program read
its own source, where the `#define` lines are ignored and the shape of the
code body is decoded as Braille. That is why `./huffman < huffman.c` prints
a message.
