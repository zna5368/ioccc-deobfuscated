# 2018/burton1

Difficulty: Medium

1. Confusing variable names

`char O,o[];` uses the capital letter `O` for the current column and the
lowercase letter `o` for the line buffer. Both look like the digit `0`, so
expressions such as `O[o]`, `-~O` and `!O` are hard to read. In simple.c they
are called `column` and `ascii_line`.

2. Reversed array indexing

`O[o]` is the same as `o[O]`. In C, `a[b]` just means `*(a + b)`, and since
addition does not care about order, the index and the array can be swapped.

3. Testing for end of file with bitwise NOT

`~l` and `~(l=getchar())` are used instead of `l != EOF`. `EOF` is `-1`, and
`-1` has every bit set, so `~(-1)` is `0` (false). Every other value gives a
non-zero (true) result. The loop condition `for(;~l;...)` works at the start
because `l` is really `argc`, which is `1` when no arguments are given.

4. Incrementing with `-~O`

`-~O` is the same as `O + 1`. In two's complement arithmetic `~O` equals
`-O - 1`, so negating it gives `O + 1`.

5. Using the return value of printf as a constant

`-~O&printf("%02x ",l)*5` prints the byte in hex and also computes the next
column. `printf("%02x ",l)` always prints 3 characters and returns 3, so
`3*5` is `15`. Then `(O + 1) & 15` wraps the column back to 0 after 16 bytes.
One expression does both the printing and the counting.

6. Bit tricks for the printable test

`4<(4^l>>5)?l:46` decides whether a byte is shown as itself or as a dot (`46`
is the ASCII code for `.`). `l>>5` divides the byte by 32, giving 0 to 7.
XOR with 4 maps 1, 2 and 3 to 5, 6 and 7, and every other value to 4 or less,
so the test is true only for bytes 32 to 127. simple.c writes this as
`c >= 32 && c <= 127`.

7. Nested ternary operators in one statement

The whole program is a single assignment inside a `for` loop:
`O=(O[o]=~(l=getchar())?...:0)?...:!O`. It reads a byte, stores the sidebar
character (or a `0` terminator at end of file), prints the hex value and
updates the column, all in one line. At end of file the stored `0` is false,
so the last branch `!O` runs: it makes the column 0 when a partial line is
waiting (so it gets printed) and 1 when the line is empty (so nothing is
printed). simple.c splits this into an `if` for end of file and separate
statements for the normal case.

8. Work hidden in the for loop increment

`for(;~l;O||puts(o))` prints the ASCII sidebar in the increment part of the
loop. `O||puts(o)` uses short-circuit evaluation: `puts` is only called when
`O` is 0, meaning a full line (or the final partial line) is ready.

9. Implicit int and implicit declarations

`main(l)` has no return type and no type for `l`, and there are no
`#include` lines, so `getchar`, `printf` and `puts` are used without being
declared. Old C rules assume all of these are `int`. The parameter `l`
is really `argc`, reused as the variable that holds each input byte.

10. Zero size array that overflows

`char o[];` creates an array of only one byte, but the program writes up to
17 bytes into it. It relies on the empty, zero filled memory after the array.
This is undefined behavior: with optimization turned on (such as the `-O3` in
the Makefile), modern gcc assumes the index must be 0 and the sidebar only
shows the first character of each line. simple.c gives the buffer its real
size of 17 bytes, which matches the original built without optimization and
the alternate version prog.alt.c.
