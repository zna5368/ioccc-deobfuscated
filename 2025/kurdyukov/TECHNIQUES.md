# 2025/kurdyukov

Difficulty: Hard

1. Shaped source layout

The code is drawn as a picture: a narrow column of `int main ( int c, char**v)`
on top, then a wide block, then a large "X" made of diagonal lines. Spaces and
line breaks are placed only to make the shape, so statements are cut in the
middle (for example `r[` ... `x-l]=b`). Reformatting the code with normal
indentation removes this.

2. One array split into many hidden tables

`int s[5<<15]` with `l=1<<15` holds five tables of 32768 entries each. `s[x]`
is the length of the best expression for `x`, `s[x+l]` (written `u[l]`) is its
operator, `s[x+2*l]` (`u[l+l]`) the left operand, `s[x+3*l]` (written
`r[x-l]`, a negative index from the fifth table) the right operand, and
`r=s+l*4` is the list of reached values. In simple.c these are separate arrays
`length`, `operation`, `left_operand`, `right_operand` and `found`.

3. Single letter and reused variables

`c`, `v`, `q`, `p`, `o`, `d`, `b`, `x`, `z`, `n` change meaning between parts
of the program. For example `d` is first an input number, then the larger
operand, and in the printing loop it is a state number (1, 2 or 3). `q` is the
loop index during the search and the current value when printing. `o` is the
operator number while searching and the "context" while printing.

4. Whole program written as for loops with comma and ternary expressions

There are no `if`, `while` or function calls of its own. All the work happens
inside `for(...;...;...)` headers using `,` to chain statements and `?:` for
choices, e.g. `x&&x<l&&(u=s+x,!*u?r[n++]=x:*u>z)?r[x-l]=b,*u=z,u[l]=o,u[l+l]=d:b`
means "if the result is valid and either new or shorter, remember it". Even
the body of a loop is placed in the increment part. simple.c uses `if`,
`switch` and a helper function `consider()` instead.

5. Hidden min/max swap

`d=d<b?b=d,x:d` puts the larger of two numbers in `d` and the smaller in `b`
using a ternary with a comma inside it. simple.c computes `big` and `small`.

6. Operator coded as a counting number

The inner loop `o=5;--o;` runs `o` = 4, 3, 2, 1 for `-`, `+`, `/`, `*`, and a
chain `o<2?d*b:o>3?d-b:o<3?...:d+b` picks the operation. The order matters
because ties keep the first found expression. simple.c uses an `enum` and a
`switch`.

7. Arithmetic tricks to reject results

Division is written `d/b+d%b*l`: if it does not divide evenly the result
becomes larger than the limit and is thrown away by the `x<l` test. Subtraction
of equal numbers gives 0, which fails the `x&&` test. In simple.c the
divisibility test is written out.

8. Hidden cost for parentheses

`z+=(s[b+l]/3+s[d+l]/3)*o` is done only when `o` is 2 (divide), and the extra
length stays for the following multiply. `op/3` is 1 only for `+` and `-`, so
this adds 2 characters for each operand that needs parentheses.

9. Recursion replaced by a hand-made stack and state machine

The printing loop walks the expression tree without a recursive function. It
pushes `q`, `o` and the next state `d` onto the reused `r` array, and pops them
with `*--r`. A 0 stored at the bottom of the stack marks the end of an
expression. `d=0` means "read the next target", 1/2/3 mean "start node",
"after left side", "after right side". simple.c uses the recursive function
`print_expr()`.

10. Bit tricks for operator inversion

`o=o==(--x|1)` and `o^=x&d` decide whether a sub expression is on the right of
`-` or `/`. If it is, its operator is printed flipped with `^1` (`+` becomes `-`,
`*` becomes `/`) so parentheses are not needed, e.g. `a-(b+c)` prints as
`a - b - c`. `z=o/2<x/3` decides when parentheses are needed (a `+` or `-` part
inside a `*` or `/`). simple.c names these `inverted` and `parens`.

11. Dead code and misleading parts

`d=!q+*--r` adds `!q`, which is always 0 because a value is never 0. `n=q`
just means `n=0`, and `--c>q` compares with `q` which is 0 at that time.
`" =" " %d\n"` splits a string literal in two to fit the shape.
