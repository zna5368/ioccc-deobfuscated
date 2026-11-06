# 2019/endoh

Difficulty: Insane

1. Output hidden in a debugger backtrace

The program prints nothing and just crashes. The real output is the list of
function calls that gdb shows with `bt`. Each frame's line number is an ASCII
code, so reading the line numbers in order spells out the source code. The
only way to see the result is to run the program inside a debugger.

2. Functions placed on the line that equals their character code

`o x23(){f[*r--]();}` is on line 35 because `0x23` is 35, which is `#` in
ASCII. Every character used in the program gets its own function on its own
line, and the empty lines in between (for example lines 14 to 31) are there
only to push each function down to the right line. `x0a` (newline) and `main`
are both hidden on line 10 inside the code that follows the long string.

3. Deep recursion through a function pointer table

Every `xNN` function does the same thing: `f[*r--]()` takes the next
character from an array and calls the function for that character. This makes
a chain of about 2300 nested calls, one per character of the source. The table
`o(*f[])()={b,0,...,x7d}` on the last line maps each character code to its
function, and is written with plain zeros instead of names for unused
characters, so it is hard to see which index belongs to which function.

4. Crashing on purpose with a NULL function pointer

Before the source text, `*h=1` stores the value 1. Character 1 is never used,
so `f[1]` is a NULL pointer. When the chain reaches it, the program calls
address 0 and crashes, which freezes the whole call chain so gdb can show it.

5. One array used for three different things

`int t[9999]` is the only big array. `t[0]` to `t[126]` are flags that record
which characters appear in the source. `t[128]` and up is where the source text
is written through the pointer `h`, and `t[2097]` and up is where the table
entries are written through the pointer `r`. The magic number in
`r=2096+t` is chosen so that the text written by `h` ends exactly where the
text written by `r` starts, so the two pieces join into one long piece of
memory that `r` then reads backwards.

6. Self-describing source string (quine)

The string `d` holds a compressed copy of the program. It is split into
pieces separated by `!!`, and inside a piece `!` stands for a newline. The
function `v(x)` copies the piece that starts at offset `x`, so `v(0)`,
`v(5)`, `v(21)`, `v(57)` and `v(71)` are pieces like `o x`,
`(){f[*r--]();}`, the `#define` header, the table start and the main code.
The string is also copied character by character to reproduce itself in
quotes, adding a backslash and newline every 119 characters to match how it
is wrapped in the file.

7. Source file that is also a gdb script

The first line `#define o void/*` starts a C comment, so the lines `r`, `bt`
and `q` are ignored by the C compiler. To gdb, the `#` line is a comment and
`r`, `bt` and `q` are the commands run, backtrace and quit. This lets the same
file be passed to `gdb -x prog.c`.

8. Macro used to shorten a keyword

`#define o void` replaces the word `void` with the single letter `o`, so the
function definitions look like `o x23()` and are harder to recognize.

9. Arithmetic in place of character constants

Characters are written as numbers: `34` is `"`, `92` is `\`, `10` is a
newline, `44` is `,`, `48` is `0` and `125` is `}`. `*r+=72` turns `0` (48)
into `x` (120). `x+48+x/10*39` turns a number from 0 to 15 into a hex digit
by adding 39 extra for values of 10 and above to jump from the digits to the
letters `a` to `f`.

10. Strange loop conditions and comparisons

`*q+q[1]-66` is zero when the two characters add up to 66, which is how `!!`
(33 + 33) is found. `*q-33?*q:10` turns `!` (33) into a newline.
`++n/126-1` keeps the loop going while `n` is below 126, and `n/13?...:0`
only adds a newline once `n` is 13 or more.

11. Undoing work by moving a pointer back

When `n` is 10 the loop still writes the function `o x0a(){f[*r--]();}`, then
`h-=n-10?0:19` moves the write pointer back 19 characters to erase it, because
`x0a` is already defined on line 10.

12. Comma operators and nested ternaries as statements

The loop body `*++r=44,*++r=48,t[n]?*r+=72,v(0),...,h-=...:h;` packs several
statements into one expression using commas, and uses `?:` instead of an
`if` statement.
