# 1995/esde

Difficulty: Medium

1. Variables named after library functions

Local variables and parameters are named `atoi`, `fgetc`, `rand`, `qsort`,
`log`, `abs`, `cos`, `pow`, `sqrt`, `exp`, `sin` and `malloc`. A local name
hides the library function of the same name, so `qsort[cos]` is just an array
element and `malloc` is just an `int` holding a string length. The reader
keeps thinking functions are being called. simple.c uses names such as
`line`, `word`, `wanted`, `found` and `length`.

2. XOR instead of assignment

Almost every assignment is written with `^=`. `x ^= x` sets `x` to 0, and
`x ^= y ^ x` sets `x` to `y`, because XOR-ing a value with itself cancels out.
For example `abs ^= strlen(log) ^ abs` means `abs = strlen(log)`, and
`cos++[qsort] ^= abs++[log] ^ qsort[cos]` copies one letter into the word
buffer. The comparison of two Soundex codes is also done with XOR:
`fgetc[0]^rand[0] | fgetc[1]^rand[1] | ...` is zero only when all four
characters are equal.

3. Reversed array subscripts

C defines `a[i]` as `*(a+i)`, which is the same as `i[a]`. The code writes
`abs[log]`, `cos[qsort]`, `exp[sqrt]` and `sin[sqrt]` instead of `log[abs]`
and so on. `pow[2][pow]` is even `pow[pow[2]]`, which is `pow[0]` because
`pow[2]` was just set to 0.

4. Constants hidden as character arithmetic

Numbers are written as XORs of characters. `'C'^'c'` is 32 (the array size
and the upper/lower case bit), `'^'^'^'` is 0, and `0x40` and `0x0100` are
`'@'` and 256. simple.c writes the plain values.

5. Soundex table encoded with symbols

The string `"__+C-_+C__CC*//_+C&C-_+_C_C/*__"` is the Soundex table, looked
up with `letter ^ 0x40` so that `'A'` is index 1. Instead of the digits 0 to
6 it uses the symbols `_ + C - * / &`. A comment in simple.c lists which
letters give which symbol. Because code 2 is the letter `C`, a word starting
with `C` drops a following code 2, which simple.c keeps.

6. Confusing ways to set simple values

The start of the code buffer is filled with `pow[1]^=pow[1]; pow[2]&=pow[1];
pow[3]^=pow[3]^pow[2]; pow[0]^=sqrt[0]^pow[2][pow];`, which only sets
`pow` to the first letter followed by three zeros. The length check
`(malloc ^= strlen(sqrt) ^ malloc, malloc ? malloc : malloc ^ malloc) ^ 1`
uses a comma operator and a `?:` whose two branches give the same value, and
just means `length = strlen(word); if (length == 1) return;`.

7. Disguised loop conditions

`!!(~exp&3)` is true until `exp` reaches 3, so the final loop stops after
three code digits have been added. The upper case conversion uses
`c>='a' ^ c>'z'`, which is true only for `'a'` to `'z'`, and then XORs in 32
to clear the lower case bit.

8. Dead code

`if(!(0xEf^abs[log]))continue;` is the body of a `while` loop, and
`continue` there does the same as doing nothing, so the check has no effect.
simple.c removes it.

9. Implicit declarations and ASCII art layout

Only `<stdio.h>` is included, so `strlen`, `isalpha` and the function `SD`
are used without declarations and `main` and `SD` default to returning `int`.
The source is laid out in a picture with tokens split across lines, which
hides where each statement starts and ends.
