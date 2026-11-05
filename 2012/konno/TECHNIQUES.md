# 2012/konno

Difficulty: Medium

1. Recursion in place of a loop

`main` calls itself once for every character printed, so the function is
really a loop that runs 512 times. The global `_` is the loop counter, and
each call passes the loop's state to the next one through its arguments.

2. Short-circuit operator in place of an if statement

`_++>>9||main(...)` is the loop test. `_++>>9` divides the counter by 512, so
it stays 0 until the counter reaches 512. The `||` only evaluates its right
side when the left side is 0, so `main` stops calling itself at that point.

3. A whole row stored in one integer

`O*2` moves every bit of `O` one place to the left and `1&(...)|` puts a new
bit in the empty lowest place. This makes `O` a shift register, a queue of the
last 32 cells. A row takes exactly 32 steps (31 cells and a newline), so when a
cell is about to be printed, the top three bits of `O` are the cell above it
and that cell's left and right neighbours. No array is needed.

4. Printed character reused as data

`putchar` returns the character it printed, and `%10` turns that into a small
number: `#` (35) becomes 5, a space (32) becomes 2, and a newline (10) becomes
0. This number is passed to the next call as `o`. Its lowest bit tells whether
the cell was alive (odd) or dead (even), and 0 means a new row just started.
The program reads back what it printed rather than keeping a separate variable.

5. Parameters of main used as ordinary variables

The names hide that `O` starts as `argc` and `o` starts as the environment
pointer, a third argument to `main` that most programs ignore. Neither start
value matters. The pointer is not 0 and its lowest bit is 0, so the first step
behaves like "the last cell was dead". The bits of `argc` are pushed out of the
top of `O` before the second row, the first one that uses `O`, which is why
extra command line arguments change nothing. In the recursive calls, the same parameters hold the
register and the last character.

6. Edge of the row handled by a hidden shift

`7&O<<!o>>!o+29` picks the three cells above the current one. `!o` is 0 in the
normal case, so this is just `7&O>>29`, the top three bits. `!o` is 1 for the
first cell of a row, which makes it `7&(O<<1)>>30`. The left shift drops the
top bit, which belongs to the end of an older row. Shifting a signed number
right copies its top bit into the empty places, so the bit for the cell above
appears twice. The left neighbour is replaced with a copy of the cell itself.
This is the difference from `7&O>>29` that the author asks about.

7. Newline slot filled with a copy of the last cell

`1&(o?(int)o:O)` chooses the bit to push. Usually it is the bit from `o`, but
the newline has `o` equal to 0, so the `?:` falls back to `O`, whose lowest bit
is the last cell of the row. The 32nd slot of the row therefore holds a copy of
the last cell, and that copy acts as the right neighbour of the last cell. This
works together with technique 6 so that both edges of the row copy their
nearest cell.

8. Rule table lookup with a shift

`atoi(1[l])>>(...)` uses the three cells as a number from 0 to 7, and then
shifts the rule number right by that amount. The lowest bit of the result is
the rule's answer for those three cells. The rule number itself is the lookup
table, and it is parsed again for every character instead of being stored.

9. First row built from comparisons

`...&32<_|_==16` has no separate setup for the starting row. `32<_` is 0 during
the first row, so the `&` forces every cell to be dead, and `_==16` then makes
just the middle cell alive. After the first row, `32<_` is 1 and the `&` keeps
only the lowest bit of the shifted rule, which is exactly what technique 8
needs.

10. One counter for rows and columns

There are no separate row and column variables. `_%32` is the column, and is 0
when a newline is due. `_==16` and `32<_` compare the same counter to find the
middle of the first row and the end of the first row.

11. Relying on operator precedence

`atoi(1[l])>>(...)&32<_|_==16?35:32` has almost no parentheses. C groups it as
`(((rule>>shift)&(32<_))|(_==16))?35:32`, and `7&O<<!o>>!o+29` groups as
`7&((O<<!o)>>(!o+29))`. A reader has to know the precedence of every operator
to see what is being computed.

12. Characters written as numbers

`35`, `32` and `10` are the ASCII codes for `#`, a space and a newline. Using
numbers hides that they are characters and makes the `%10` in technique 4
harder to notice.

13. Swapped array index

`1[l]` is the same as `l[1]`, because `a[b]` means `*(a+b)` and addition works
in either order. Here it is just `argv[1]`.

14. Confusing names

`_`, `O`, `l` and `o` were chosen because they look alike or look like the
digits 0 and 1, and none of them says what the variable is for.

15. Old style C with nothing declared

There are no headers and no return type, and `main(O,l,o)char**l,**o;` is the
old way to declare parameters. Because this style turns off argument checking,
the program can pass a plain `int` to `o`, which is declared as a pointer, and
call `atoi` and `putchar` without declaring them.
