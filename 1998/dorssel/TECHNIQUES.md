# 1998/dorssel

Difficulty: Hard

1. One array holds the lookup table, the work space and the input line

`char l[999]` is used for several jobs. `l[0]` is a scratch byte. `l[1..10]`
holds the digit codes and `l[18..43]` the letter codes. `l[11..14]` is the
string `" .-"` (and `l+12` is `".-"`) used by `strspn`. `l[44]` is a zero byte,
and the input line is read into `l+45`. The clean version uses separate arrays:
`digit_codes`, `letter_codes` and `line`.

2. Morse code packed into the bits of a printable character

Each character in the table string is a Morse code. Subtract 32 and look at the
bits from the lowest one up: a 1 bit is `.`, a 0 bit is `-`, and the code stops
when only binary `10` is left. For example `')'` is 41, and 41 - 32 = 9 =
binary `1001`, which reads `.` then `-`, the Morse code for `A`. The clean
version writes the numbers out with the Morse code in a comment next to each.

3. Strange index math to find a table entry

`(*O&31)[l-15+(*O>61)*32]` uses `a[b] == b[a]` to hide an array lookup.
`*O&31` turns `'A'` and `'a'` into 1 and `'0'` into 16. `(*O>61)*32` adds 32
only for letters. Together they point to `l[1..10]` for digits and `l[18..43]`
for letters. The clean version uses `c - '0'` for digits and `(c & 31) - 1` for
letters.

4. Encoding the dots and dashes with arithmetic

`putchar(45+*l%2)` prints `'-'` (45) for an even number and `'.'` (46) for an
odd one. `(*l=*l+32>>1)>35` shifts the code right by one bit while keeping the
extra 32 in place, and stops when only the end marker is left. The clean version
uses a `do`/`while` loop with `code & 1 ? '.' : '-'` and `code >>= 1`.

5. The mode flag is hidden in the table

`*l=O[strspn(O,l+11)]` stores the first character of the line that is not a
space, dot or dash. If there is none, the byte is zero and the line is decoded
from Morse; otherwise it is encoded to Morse. The same byte `*l` is then
overwritten while encoding each letter, but it never becomes zero, so the mode
stays the same. The clean version uses a separate `to_morse` variable.

6. A switch on a computed value with cases inside a block

`switch((*l&&isalnum(*O))-!*l)` gives -1 for decoding, 1 for a letter or digit,
and 0 for any other character when encoding. `case 1:` is placed inside the
`{ }` block of `case -1:`, after its `break`, so jumping to `case 1` skips the
decoding code and runs the encoding code. Case 1 then falls through into
`case 0`, which prints the space. The clean version uses plain `if`/`else`.

7. A variable that hides another variable with the same name

Inside the decoding block, `char*I=...,O=34;` declares a new `char O`. This
hides the pointer `O` for the rest of the block, so `O` there is the code being
built and not the input pointer. The clean version calls them `text` and
`code`.

8. Decoding by reading the Morse code backwards

`I` points to the last dot or dash of a sequence and walks backwards with
`*I--`. `(O-16<<1)+*I-'-'` doubles the code and adds 1 for `.` (46 - 45) and 0
for `-`. Starting from 34 (32 + the marker 2), this rebuilds the packed number.
`*I&3` is nonzero only for `.` and `-`, so it stops at the space or zero byte
before the sequence. The clean version loops over the symbols from last to first
and uses `== '.'`.

9. Bit tests that stand for simple comparisons

`O&93` is zero only when the code is still 34, meaning no dots or dashes were
read, so a space is printed. `*I&8` is nonzero only for `.` and `-`, meaning
the sequence was longer than 5 symbols, so `?` is printed. The clean version
checks the length of the sequence directly.

10. Pointer math to turn a table position into a character

`memchr(l,O,44)` finds the code in the table and `I-l+47` turns its position
into a character: positions 1 to 10 become `'0'` to `'9'` and positions 18 to 43
become `'A'` to `'Z'`. The clean version searches each table and returns
`'0' + i` or `'A' + i`.

11. Hidden newline removal and character constants as numbers

`strlen(O)[O-1]=0` is `O[strlen(O)-1]=0`, which removes the newline. It is
placed inside the index of another expression with the comma operator.
`putchar(10)` and `putchar(32)` print a newline and a space, and
`putchar((++O,32))` also moves the pointer forward. The clean version uses
`'\n'`, `' '` and separate statements.
