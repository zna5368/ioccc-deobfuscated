# 2001/cheong

Difficulty: Hard

1. Subtraction-only longhand square root

The whole program is the schoolbook "longhand" square root, but written so
that it never multiplies or divides to guess a digit. For every pair of input
digits, the odd numbers `20*root+1`, `20*root+3`, `20*root+5`, ... are taken
away from the remainder until it would become negative. The number of
subtractions that worked is the next root digit. The `while(!pain(0,O,l))D[l]+=20;`
loop is this repeated subtraction, and adding 20 to `D[l]` moves on to the next
odd number.

2. Two numbers packed into one string

The program has no arrays of its own. It reuses the argument string `O[1]`
and stores two numbers in it at once. Every character holds a digit of the
remainder (read with `(D[I]+82)%10`) and the odd positions also hold a doubled
root digit in the upper part of the value (read with `(D[I]+72)/10`). Because
the root has half as many digits as the remainder, the root digit for place
`I` is found at `D[I-l+I]`, i.e. every second character.

3. Hybrid base 10 and base 20 digits

A character's value is roughly `20*root_digit + remainder_digit`. Adding 20
bumps the root digit without changing the remainder digit, and `%10` / `/10`
pull the two parts apart again. The value `2*digit+1` used as the lowest
subtrahend digit can be as large as 19, so the "digits" are not ordinary
base-10 digits.

4. Subtraction done like addition

`c=o+...-9` and `(c+999)%10` turn a subtraction with a borrow into something
that looks like addition: `4 - 8` becomes digit `6` with a carry of `-1`. The
extra `-9` makes C's divide (which rounds toward zero) give the right borrow
in `c/10`, and the `+999` keeps the `%10` from seeing a negative number.

5. Recursion to walk digits and undo failed work

The subtraction is done by `pain` calling itself with `I-1`, one call per
digit, from the right end to the left. The borrow comes back as the return
value. Only when the final borrow is 0 does each call on the way back out
actually store its new digit, thanks to `!(o=pain(...))*(...)`, which
multiplies the change by 0 when the subtraction failed. The recursion ends
when `I` goes below zero, where the code even reads the byte before the
string.

6. One function with two jobs

`pain` is both the printing loop (when `o>0`, the argument count passed in from
`main`) and the digit subtracter (when `o` is the borrow, 0 or negative). The
same variable `o` therefore means the argument count, the incoming borrow and
the success flag returned to the caller. `main` just forwards to it, so the
program's exit status is the argument count.

7. Shifted, meaningless constants

The digits are prepared with `D[l++]-=120` and `D[l]-=110`, and read back with
offsets like `+82`, `+72`, `+92` and `+1032`. These cancel out the ASCII code
of `'0'` and set up the packed encoding, but they were chosen to hide that:
for example `(D[l]+1032)/20` is just `'0' + root_digit`. Even the loop
increment `D[l++]-=10` is a hidden step that turns the last trial value
`2*digit+1` into the stored doubled digit `2*digit`.

8. Odd names and shaped layout

The names `o`, `O`, `I`, `l`, `D` and `pain` look alike and say nothing, and
`putchar(10)` hides a newline. The source is squeezed into a rectangle, and the
gaps of whitespace inside it are meant to draw a square root sign, which also
splits tokens across lines and makes them hard to read.
