# 2024/kurdyukov4

Difficulty: Hard

1. Token pasting macro to hide keywords and function names

`#define o(x,o)x##o` glues two pieces of text into one token, so
`o(spr,intf)` becomes `sprintf`, `o(prin,tf)` becomes `printf`,
`o(get,char)` becomes `getchar`, `o(ret,urn)` becomes `return` and
`o(u,nsigned)` becomes `unsigned`. Using `o` as both the macro name and one
of its parameter names makes it even more confusing. A search for `printf`
will not find these calls.

2. Code shaped into a picture (a spiral)

The source is laid out as a spiral. Strings are split into tiny pieces like
`"#inclu" "de " "<st" "dio" ".h>"`, which C joins back together because
adjacent string literals are concatenated. Tokens are split across lines
wherever whitespace is allowed, so statements are hard to follow.

3. Single letter globals and reused locals

`X` is the table of variable names, `Z` is the output text buffer, `N` is
the last name created, `P` is the write position, `H` is a saved position and
`A` is a temporary expression buffer. Inside `C()` the locals `a`, `b`, `c`
first hold the square root and remainder, then get reused to hold the three
characters of the previous name.

4. Numbers written as characters and characters written as numbers

ASCII codes appear as plain numbers: `45` is `'-'`, `43` is `'+'`, `65` is
`'A'`, `90` is `'Z'`, `97` is `'a'`, `98` is `'b'` and `48` is `'0'`. Some
places use the character form (`'z'`, `'j'`) right next to the number form,
so you have to translate each one to see what the code compares against.

5. Chained assignment to initialize several things at once

`int a,i= **X=Z[1]='a'` stores `'a'` in the first name slot (`**X` is
`X[0][0]`), in `Z[1]`, and in `i` all at once. Giving `i` the value 97 just
makes the first `i>3` loop test true, and the `Z[1]` store is never needed
because that byte gets overwritten later.

6. One huge expression using the ternary and comma operators

The code that makes the next variable name is one expression of nested
`?:` operators with assignments and comma operators inside it, like
`(b='z'-b?b-90?b%64+65:97:(c=c?'z'-c?-~c:48:'a',65),'j')`. It works like an
odometer: it moves the first letter forward and, when that wraps around,
moves the second letter (and then the third) forward. Tests like `'z'-b`
(true when `b` is not `'z'`) and `a+!b-90` (checks for `'Y'` in a one letter
name or `'Z'` in a longer one) hide plain comparisons. Rewriting it as
`if`/`else` statements shows the logic.

7. Bit tricks for simple arithmetic

`-~c` means `c+1` (`~c` is `-c-1`). The generated program uses the same
trick to get the number one with no literal: `int b=-~a` where the global `a`
is zero. `(x>>16)` checks whether `x` is 65536 or bigger, and `65535&b` keeps
only the low 16 bits.

8. A recursive function that does three jobs

`C(i,x)` returns the name or expression for the number `x`, prints a new
variable declaration as a side effect the first time a small number is
needed, and when `i>0` also appends the result to the `Z` buffer. Each number
is written as `r*r+d` or `r*r-d`, where `r` is the rounded square root and `d`
is what is left over, and `C` calls itself to build `r` and `d` too.

9. Hidden string ending with `%c`

`sprintf(A,"%s*%s%c%s",...)` passes the character `0` when the leftover is
zero. That writes an end-of-string byte in the middle of `A`, so everything
after `r*r` is cut off without any `if` statement.

10. Loop with the real work in the increment part

In `for(printf(...); i>3&&...; C(i,a)) for(...) ...` the first part prints
the program header, the body reads four input bytes into an `int` through a
`char *` cast, and the step `C(i,a)` handles each chunk. The loop really
acts like a `do`/`while` loop.

11. Overwriting a separator to split a buffer

`H=P;C(1,n);*H++=0;` appends the byte count to the end of the value list and
then changes its leading comma into an end-of-string byte. Now `Z+1` is just
the list of values and `H` points at the length, so one buffer gives two
strings. Starting at `Z+1` also skips the comma at the front of the list.

12. Integer wrap-around used on purpose

For numbers close to 2^32 the rounded square root is 65536. `65535&b` turns it
into 0 and `b*b` wraps around to 0, so the expression becomes `a*a-d`, a
negative `int` with the same bits as the original unsigned value. The
generated program also passes `Z`, an `int` array, to `fwrite` with an item
size of one byte (`b`), so it writes the raw bytes of each `int`.
