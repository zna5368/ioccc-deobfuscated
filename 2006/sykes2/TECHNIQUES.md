# 2006/sykes2

Difficulty: Hard

1. Recursive `main` as a loop

`_^448&&main(-~_,v);` makes `main` call itself until its argument reaches 448.
`main` first gets `argc` (1), so the calls nest 448 deep. The printing happens
after the recursive call returns, so the deepest call prints first. The program
counts from 447 down to 0 without writing a loop, and the counter always holds
the row and column together. `simple.c` uses two plain `for` loops (rows and
columns) instead.

2. Combined row/column counter split with bit tricks

The one number `_` (0..447) holds everything about the current screen position.
`_%64` is the column (and `_%64 == 0` prints a newline instead of a character),
`_/64` is the row, `_/8%8` picks which of the 8 time characters is drawn,
`_&4` picks the left or right half of a digit cell and `_&2` picks one of two
3-bit fields. Because the counter runs downward, the screen is drawn right to
left and bottom to top in counter terms, so every index has to be flipped
(`7[...-_/8%8]` means `__TIME__[7 - _/8%8]`).

3. `x - y` and `-~x` instead of plain arithmetic

`-~_` is `_ + 1` (`~_` is `-_-1`, and negating it gives `_+1`). It is used to
increment the counter and also to add 1 to the segment byte before the bits are
tested, which changes which bits in the lookup strings mean which segments.
`_^448` is used as a "not equal to 448" test, because XOR gives zero only when
both sides are the same.

4. Reversed array subscripts

In C, `a[b]` is the same as `b[a]`. `7[__TIME__-_/8%8]` reads as an index into
the number 7, but it really means `(__TIME__ - _/8%8)[7]`: a character from the
time string. The next subscript `[">'txiZ^(~z?"-48]` does the same trick with
pointer subtraction: it moves the string pointer back by 48 (`'0'`), so a digit
character like `'5'` can be used directly as the index.

5. Data hidden in string literals

`">'txiZ^(~z?"` is a table of 11 bytes, one for each of `0`..`9` and `:`.
After adding 1, each bit of a byte tells whether one segment of the seven
segment display is lit. `";;;====~$::199"` is a second table that says which
segment covers each 2 column block of each row; each byte holds two 3-bit
segment numbers (read with `/1%8` or `/8%8`). As the author points out, the
hidden null byte at the end of that string is also part of the table. Segment
number 7 never matches any bit, so it is used for the blank gap between digits.
`simple.c` replaces both strings with named segment bit masks and a readable
`layout` table.

6. Operator precedence instead of brackets

`32|-~a>>b/c%8&1` has almost no brackets. It relies on the order C evaluates
operators: unary `-~` first, then `/` and `%`, then `>>`, then `&`, and `|`
last. Written out it is `32 | (((a + 1) >> ((b / c) % 8)) & 1)`. The result is
either 32 (a space) or 33 (`!`), because `32|1` is 33.

7. `__TIME__` macro for input

The program takes no input. The time it prints is the time it was compiled,
which the preprocessor supplies as the string `"HH:MM:SS"`. That is why
`try.sh` deletes and rebuilds the program every second.

8. K&R style `main` and implicit declarations

`main(_,v)char**v;` uses the old K&R way of declaring parameters, the return
type of `main` is left out (implicit `int`), `_` is used as a variable name,
and `putchar` is called without including `<stdio.h>`.
