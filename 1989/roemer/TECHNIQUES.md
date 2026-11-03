# 1989/roemer

Difficulty: Medium

1. Misleading theme

Every name and number is made of the digits of pi (`_3141592654`, `_314159`,
`__3_1415`, `3141`, `31415`), and the code is shaped like the letter pi. The
judges add "more here than meets the PI". The program does not compute pi at
all; it adds up `1/0! + 1/1! + 1/2! + ...` to print e = 2.71828... The pi theme
only exists to send the reader the wrong way.

2. Identifiers that differ only by underscores

Variables such as `_314`, `_3141`, `__3141`, `_31415` and `__31415` look almost
the same. They are the remainder, the remainder table, the term array, the loop
counter and the divisor `n`. In `simple.c` they are `rest`, `remainders`,
`term`, `count` and `n`.

3. Numbers built from pi digits

Plain constants are written as arithmetic on pi digits: `31415/3141` is `10`,
`3141/314` is `10`, `31415%314-(3,14)` is `15-14 = 1` (standard output),
`(3,14-4)` is `10`, and `"314"[3]+1` reads the hidden `\0` at the end of the
string and adds 1 to get `1`.

4. Comma operator hiding the real value

`(3,1)` evaluates `3`, throws it away, and gives `1`. `(--*__3_14,__3_14)`
first changes a character and then gives the pointer. `(_3_14159++,++_3_14159)`
turns the flag `0` into `2`, which is used as the length `2` of `write`.

5. Reversed array indexing

`a[i]` is the same as `i[a]` in C, so `_31415[_3141]`, `_314[_3141]` and
`1[__3141]` are ordinary array accesses written backwards. Worse,
`_3141592654[0][_3141592654-1]` uses the value `2` just stored in element 0 as
the index into `_3141592654-1`, which is element 1 of the sum.

6. Digit to character by pointer arithmetic

`_3141592654[_31415] + "0123456789"` adds a digit value to the address of the
string `"0123456789"`, so `write` prints the matching character. `simple.c`
keeps this but puts the string first.

7. Changing a string literal

The program prints `"3.1415"` but first runs `--*__3_14`, which turns the `3`
into a `2`, and only writes 2 characters: `"2."`. This hides the fact that the
answer starts with 2. Modern systems keep string literals in read only memory,
so this line crashes both the original and `simple.c` before anything is
printed. Copying the string into a `char` array makes both programs print e.

8. Unused results

`write(...) + 3.1415926`, `write(...) - _314` and the final `_314 =
*"3.141592"` compute values that are never used. They only add more pi to the
code and are removed in `simple.c`.

9. Hidden lookup table

`_314159` and `_3141` cache `r/n` and `r%n` for every value of `r` that long
division can reach, and are reset to `-1` for each new `n`. This looks like a
complicated algorithm but is only a cache for a plain division.

10. Rounding outside the array

After the inner loop the digit pointer points one past the end of the sum. The
rounding step `while(++*_3_141==3141/314)*_3_141--=0;` therefore increments a
byte that is not part of the array. `simple.c` keeps this to stay the same as
the original.

11. Puts of an emptied string

`puts((*_3141592654=0,_3141592654))` sets the first byte of the sum to `0` and
then prints it, so `puts` prints an empty string followed by the newline.
