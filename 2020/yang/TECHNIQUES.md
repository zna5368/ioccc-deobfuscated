# 2020/yang

Difficulty: Insane

1. Custom bytecode for the output lines

The strings passed to `O()` such as `"ouoo9oBotoo%]#..."` are not printed as
they are. Each character is a small instruction. The function `O()` adds up
the characters (an `o` adds 82 to the next one) modulo 185 to get a number `z`.
`z / 37` picks a directive keyword (`ifdef`, `define`, `undef`, `ifndef`) and
`z % 37` picks a macro name, or the number picks a whole line from a table,
or one of the two `printf` templates. So one or two characters stand for a
whole line such as `#ifdef q`. Spaces are skipped so the strings can be bent
into the picture.

2. Reversed and substituted string table

All the text the program prints lives in the single array `n` and is written
backwards (`"edulcni#"` is `#include`). At startup a loop copies it from the
back to the front, which reverses it, and turns the letters `Q R S T U` into a
newline, space, `(`, `)` and `,`. The letter `V` becomes the end of a string,
and a pointer to each piece is saved in `t[]`. The first piece `"Average?!n"`
is a decoy that is never used.

3. Stringizing macro to hide strings

`#define C(c)#c` turns its argument into a string, so `C(Average?!nQVQ...)`
becomes `"Average?!nQVQ..."`. This lets the strings appear in the source
without quote marks and spread across many lines like ordinary code.

4. Recursive main

`main(0,e)` is called from inside the input loop. With `argc` equal to 0 the
setup code is skipped and `main` jumps to the label `O:`, where it builds a
byte out of 8 bits of the LFSR state `d` and returns it. The same label is
also used with `goto` to leave after printing the usage message.

5. Bytecode skip counter

The global `k` is set to `112-j*7` before the last big string is printed.
While `k` is above zero, `O()` decodes each instruction but throws it away.
This drops the unused top bits of the step counter (blocks of 7 lines each)
without needing a second string.

6. Hidden linear feedback shift register

The line `x=d^d/4^d/8^d/32,d=(d/2|x<<15)&65535` is a 16-bit LFSR (a simple
pseudo-random number generator) written with division instead of shifts.
The program steps it until its output matches each input byte and only
writes how many steps it took. The generated program repeats the same LFSR
with nested `#ifdef` blocks, so it only gets the right bytes back when it is
compiled with the same PIN.

7. Global variables reused for many purposes

Single letter globals such as `x`, `y`, `i`, `a` and `b` are reused for
different jobs. `i` holds the time, then a loop index, then the input
character, then a bit number. `x` and `y` are changed inside `O()` and `Q()`
as well as in `main`, so the reader has to track which use is current.

8. Dense conditional expressions

`O()` is one `for` loop whose body is a chain of `?:` and comma operators.
The test, the update of `z`, the skip counter and three different kinds of
`printf`/`puts` calls are all one expression, and `z` is reset to 0 by the
final `,0` of the comma list.

9. Shuffles that look like ordinary loops

`Q()` is a Fisher-Yates shuffle written as `for(;--p>1;q[p]=y)` with the
swap split between the loop body and the loop update. The `-~p` trick means
`p+1`. It is used to randomize which state bits make a byte, the one-letter
macro names (`w[i]+=w[i]<26?97:39` turns 0..35 into `a..z` and `A..J`), and
the order of the `#define` lines for each byte.

10. Shaped source and misleading comments

The code is laid out as a picture of the character Mile. Comments such as
`/**/` split keywords like `typedef/**/int` so the shape can be kept, and
`typedef int(I)` renames `int` to `I`. The comments `size=3173` and
`crc=b7f9ecff` are part of the picture and do not affect the program.
