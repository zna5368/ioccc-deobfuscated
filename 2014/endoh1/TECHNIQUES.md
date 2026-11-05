# 2014/endoh1

Difficulty: Insane

1. Source code drawn as a picture

The source file is laid out so that its characters form a QR code. To make this work,
the long string `s` is broken into blocks with runs of spaces between them, and each
line ends in a `\` so the string can continue on the next line. Those spaces would
normally become part of the string, so the first loop throws them away:
`if(*s>32)` only keeps characters above the space character. `simple.c` has no filler
to remove, so this loop is gone.

2. Comments and do-nothing code used as padding

The picture needs a character in certain places, so some code exists only to fill them.
The comments `/*IOCCC2014...*/` at the start and end fill the corners. `5000+0+d` adds
a pointless zero, and `int main(){;for` has a stray `;`. They change nothing.

3. A program hidden inside a string

Almost everything in `prog.c` is one string, and the visible C code is only a small
unpacker. The real content is another C program that is printed when `prog` runs, so
reading the outer code tells you very little. `simple.c` splits this string into three
named parts: `qr_bytecode`, `version_adjustments` and `packed_source`.

4. A replacement character for spaces

Since all whitespace is removed from the string, a real space cannot be stored in it.
The letter `Y` is used instead, and `*s-89?*s:32` turns it back: 89 is the character
code of `Y` and 32 is a space. So `intYH[` really means `int H[`. `simple.c` writes real
spaces in its strings.

5. Hand made text compression

The source of the second program is compressed using a format the author invented, so
it cannot be read in the string. Each piece starts with a token. A token below 92 means
"copy the next characters as they are". A token of 92 or above means "copy characters
that were already written earlier", which saves space when text repeats, such as
`for(`. The compressed text has to stay in `simple.c` because the second program needs
those same bytes, but the `simple.c` comments describe the format, and the tokens have
the named constants `LITERAL_RUN_TOKEN` and `BACK_REFERENCE_TOKEN`.

6. Numbers hidden behind arithmetic and character codes

Important values are written so that they do not stand out. `x==9*9` is 81, which is the
character `Q`, and `b-->4*23` is 92, the token limit. `1152+q` is the position where the
compressed text starts in the string, `36-x` undoes an offset of 36 added to a distance,
and `x=34` is the `"` character. `simple.c` uses named constants, and the 1152 is
not needed since the parts are separate.

7. One buffer used for two jobs

The array `d` holds the output at the start and, from `q=5000+d`, a cleaned copy of the
input. The two only stay apart because the output is shorter than 5000 characters. The
pointer `p` is also used twice: first for writing the copy, then for reading it.
`simple.c` reads from the constant strings and only writes into `source`.

8. Single letter names and work done in loop headers

The names `s`, `p`, `b`, `d`, `q`, `x` and `r` say nothing about their use. The loops also
hide their work inside the `for` header. In `for(d[17]=10;x=*p++,b<92&&34<b--;*r++=x)`
the comma operator reads a character during the condition check, the condition counts
down the length, and the part that normally updates a counter is the one storing the
output. `simple.c` has plain `while` loops in small functions that have descriptive names.

9. Using a true or false result as a number

A comparison in C gives 0 or 1, and that can be used in arithmetic. The condition above
always reads one character too many at the end of a run, so `p-=b<92` steps back by one,
but only if the token was a run. A reader expects a test here and finds a subtraction.
In `simple.c`, each token type has its own branch of an `if`, so nothing needs undoing.

10. A loop that ends by changing a variable as a side effect

To put the string inside the output, the code finds the marker `Q`, then runs
`for(;*q;x=34)*r++=*q++;`. The string `s` begins with `\"`, so the copy gets its opening
quote from the data. The `x=34` replaces the `Q` being held in `x`, so the normal
`*r++=x` of the outer loop writes the closing quote. `simple.c` has the function
`append_source_string`, which writes both quotes itself.

11. A fix applied over and over

The compressed text has `@` where the newline after `#include<stdio.h>` belongs, as all
the whitespace in the string is thrown away. `d[17]=10` puts the newline in, but it is
placed at the top of the loop and runs once for every token, when only the last time
matters. `simple.c` does it once at the end and calculates 17 with `INCLUDE_LINE_LENGTH`.

12. A program written for a made up computer

The second program is not really written in C. Its logic is bytecode for a tiny invented
computer, stored as printable characters. For example `b` starts a loop, `c` loads from
memory and `M` adds. A reader sees `nsu{AntynCnuq}Bnu{` and has no way to tell that it
calculates a QR code. This bytecode must stay exact, so `simple.c` cannot rewrite it, but
it has a table of the instructions at the top and a comment for each section.

13. Machine generated code

The bytecode was produced by a compiler the author wrote, and that makes it harder to
follow than code written by hand. All functions are copied inline, so there are no
function calls. The compiler also reuses the same 21 registers for unrelated values, so
register `a` can be a counter in one place and a coordinate a few bytes later. The name of
a register tells you nothing, and it can only be understood by following the code.

14. A big table replaced by a formula and corrections

The QR standard has a large table of values for each of the 40 versions. The bytecode
calculates them with `TotalCodes * 54 / 265`, which is only roughly right, then fixes the
error with one character per version from the string `))*+,**,*)-)...`. The string looks
like random punctuation. `simple.c` names it `version_adjustments`.

15. One giant expression instead of a switch statement

The interpreter in the second program decides what each instruction does in a single
chain of `?:` operators, `n=d>76?...:d>55?...:d>9?...`, instead of a `switch`. Every
arithmetic result is also calculated first and placed in the array
`g[]={n+v,v-n,n*v,...}`, so that `g[d-77]` picks one using the instruction number. Most of
the work is wasted, but no separate case is needed for each operator.

16. Instruction numbers that double as calculations

The instruction values were picked so that the interpreter could do less work. The
increment and decrement instructions are numbers 10 and 11, and `21-d*2` gives +1 and -1
from them. `*c++%93` folds the high character codes down to small numbers. The value in
`d` is also used directly as an array index, so `H[d]` reads a register with no table.

17. Jumps found by scanning

The bytecode has no jump addresses. To jump, the loop
`for(t=0;d<4&&d>=2*!!n&&(...);)t+=v++/6-16?0:v/2%3-1;` walks forward or backward one
character at a time, adding to `t` for each block that opens and subtracting for each
block that closes, until it finds the matching end. The `2*!!n` part makes the first two
instructions jump only if the value is zero, so one line handles `if`, `else` and loops.

18. Everything in one array, and reused names

The stack, registers, memory and bytecode all live in the one array `H`, separated only by
the offsets 21, 99 and 39999. `H[*g+99]` reads memory, where `*g` is the first entry of
`g`, the sum `n+v`, so it is an address calculation that looks like an array lookup. The
parameter `n` is reused as the top of the stack, and `int v` hides the parameter `v` that
holds the command line arguments, so the same name means two things in the same function.
