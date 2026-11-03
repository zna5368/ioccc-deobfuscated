# 1995/savastio

Difficulty: Medium

1. Keywords hidden behind lookalike macros

Common keywords and functions are replaced with `#define` names made only of
the characters `l` and `1`, which look almost the same in many fonts. For
example `ll1` is `for`, `ll111` is `if`, `l111` is `struct`, `l1l1` is
`unsigned`, `lll11` is `short`, `ll11l` is `long` and `ll1ll` is `putchar`.
The constants are hidden the same way: `l11l` is `0xFFFF` and `llll` is
`1000`. Running the preprocessor turns them back into normal C.

2. Lookalike variable and field names

The variables and struct fields are also named with only `l` and `1`. The
struct `llll1` is a block of digits, `lll1l` and `ll1l1` are its next and
previous pointers, and `lll` is the array of digits. In `main`, `l1l` and
`ll11` are the two numbers that take turns holding the result, `l1ll` is the
carry, `l11` is the number being multiplied by, and `lll1`/`lllll` point at the
digits being read and written. Giving them real names like `result`, `carry`
and `multiplier` makes the code readable.

3. Code fragments hidden in macros

Larger pieces of code are hidden in macros so the same work does not look
repeated. `l1l1l(l)` allocates a new block and sets both of its pointers to
null, `l1ll1` stores the lowest 4 digits of the carry and divides the carry by
10000, and `l1lll` moves to the next output block, creating it first if it does
not exist. In simple.c these became the functions `new_block` and
`next_output_block`.

4. Prompt string hidden with XOR

The text `"Enter number: "` never appears in the source. Instead the string
`"\t\"8)>l\"9!.)>vl"` is printed one character at a time, and each character is
XORed with `'L'` before it is printed. XOR with the same key twice gives back
the original, so `'\t' ^ 'L'` is `'E'`, `'"' ^ 'L'` is `'n'`, and so on. The
loop that does this is also packed into the header of a `for` statement whose
body is empty.

5. Writing zero as `1-1` and two as `1+1`

Plain numbers are avoided by writing `1-1` for 0 (for example when setting
pointers to null) and `1+1` for 2 (the first number multiplied into the
factorial). `putchar(10)` prints a newline using its character code instead of
`'\n'`.

6. Chained assignments inside expressions

Several assignments are squeezed into one line, such as
`lll1=(ll1l=(ll11=l1l))->lll;` which sets three variables at once and then
takes the digit array of the result. Another is
`(l1=l1l)->lll[l1l->lll[1-1]=1]=l11l;` which sets the first digit to 1 and uses
the value of that assignment (1) as the index where the end marker is written.
In simple.c each assignment is written as its own statement.

7. Swapping two buffers instead of copying

Each step multiplies the current number into the other buffer and then the two
buffers swap roles. The swap is hidden among the chained assignments above, so
it is not obvious that `l1l` and `ll11` take turns being the input and the
output.

8. Nested conditional operator for printing

The `printf` format is chosen with nested `? :` operators:
`(ll)?((ll%19)?"%04d":(ll=19,"\n%04d")):"%4d"`. The first group is printed
without leading zeros, later groups are padded to 4 digits, and every 19th
group starts a new line. The comma operator `(ll=19, "\n%04d")` also resets
the counter while picking the format string. In simple.c this is an
`if`/`else` chain.

9. Unusual number representation

As the author explains, the number is stored as a linked list of blocks of
1000 four-digit buckets, kept backwards (least significant first) and ended
by the special value `0xFFFF`. The printing loop has to find that end marker
and then walk the buckets and blocks in reverse, which is hard to follow
without knowing the layout.

10. ASCII art layout

The code is drawn as two large characters, a `1` and an `l`, matching the
judges' "I know an l when I see a 1" remark. The spacing and line breaks only make the picture and
have no meaning to the compiler.
