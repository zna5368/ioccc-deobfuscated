# 2015/endoh4

Difficulty: Medium

1. The whole program lives in the `for` condition

`for(m=z=1;m*m<=y?...:...;)m++;` looks like a short counting loop. The middle part
of a `for` is normally a small test such as `i<10`, but here it holds the divisor
search, the recursive calls, the printing and the subtraction. The loop body is
only `m++`, so the place where a reader expects to find the work is empty.
`simple.c` has a normal loop body with one step per line.

2. Two loops merged into one

The same `for` first searches for a divisor and then draws the groups. The test
`m*m<=y?` chooses between the two jobs on every pass, and nothing in the code marks
the point where one ends and the other begins. `simple.c` separates them into
`search_divisors` and the `while` loop in `draw_block`.

3. A loop counter that keeps running

`m` is never reset when the drawing begins, and `m++` still runs after every group,
even though `m` has no use there. It looks harmless, because `m*m` is already bigger
than `y` and `y` only gets smaller. After about 46000 groups, however, `m*m` becomes
too big for an `int` and wraps around to a negative number. `m*m<=y` is then true
again, so the divisor search restarts in the middle of the drawing and can change
`z`. This changes the output for counts with a very large prime factor, such as
99991. `simple.c` keeps this behavior, names the step `wrapped_square`, and explains
it in a comment. It uses `unsigned` numbers for the multiplication, because overflow
of a signed `int` is undefined in C and the compiler may do anything with it.

4. Nested conditional expressions instead of `if` statements

`a?b:c?d:e` is used in place of `if` and `else` everywhere, up to three levels deep:
`m*m<=y?z=y%m?z:m:x+1?z<2?y&&f(x,0):f(z,x),...`. There are no brackets to show
which `:` belongs to which `?`, so a reader has to count them. `simple.c` uses `if`
statements.

5. The comma operator hides a sequence of statements

`z<2?...:f(z,x),putchar(...),y-=z*!!y` is three statements joined with commas. A
comma runs the left side, throws away its value, and gives the value of the right
side. This is what allows several steps to fit inside the `for` condition.
`simple.c` writes each one on its own line.

6. Values chosen to control the loop

The result of the big condition decides whether the loop goes on, and each branch
arranges to give the right answer. `z=...` continues because `z` is never 0.
`y-=...` stops when `y` reaches 0. `(f(z,y/z),0)` ends in `,0` only so that the loop
stops. There is no `break` or `return`, so the way out of the loop has to be worked
out from these values. `simple.c` uses `return` and a `while (remaining > 0)` test.

7. Short-circuit `&&` used as an `if`

In `y&&f(x,0)`, the right side of `&&` only runs when the left side is true, so this
means "if `y` is not 0, call `f`". The result of the `&&` is never used.
`simple.c` handles the case of 0 with an `if` near the top of the function.

8. One function with three hidden modes

`f` does different jobs depending on `x`. With `-1` it splits the number into two
factors, with `0` it lays things out side by side, and with anything else it stacks
rows. The value `-1` is only ever passed from `main`, and the test for it is written
as `x+1?`, which means "if `x` is not -1" because `x+1` is 0 only then. `simple.c`
names the value `SHAPE_UNDECIDED` and describes the modes in a comment.

9. Recursive calls that swap the arguments

`f(x,0)` moves the second argument into the first position, while `f(z,x)` and
`f(z,y/z)` replace the first argument with a factor. The meaning of `y` and `x` keeps
changing from one call to the next, and those names suggest a position, which they
are not. `simple.c` uses the names `count` and `row_width`.

10. Picking a character with arithmetic

`32<<!y` gives one of two characters without mentioning either. `!y` is 1 when `y`
is 0, and 0 otherwise. Shifting 32 left by one doubles it to 64, which is `@`, and
shifting by zero leaves 32, which is a space. So the only place that prints the dot
looks like it is printing a separator. `simple.c` has a separate `putchar('@')`.

11. Characters written as numbers

`putchar(x?10:...)` uses the ASCII code 10 in place of `'\n'`, and 32 and 64 are the
codes of the space and `@`. A reader has to look them up. `simple.c` uses character
literals.

12. Double negation in place of a test

`y-=z*!!y` uses `!!y` to turn `y` into 1 if it is not 0, or 0 if it is. Multiplying
by this means that `z` is only subtracted when `y` is not already 0, so `y` cannot go
below zero. It replaces an `if` with a piece of arithmetic. `simple.c` does not need
it, because the case of 0 returns early.

13. Tests that read backwards

`y%m?z:m` means "if `m` does not divide `y`, keep `z`, otherwise use `m`". The
remainder is used directly as a true or false value, so the interesting case, a
remainder of 0, comes second. `simple.c` writes `number % *candidate == 0`.

14. Implicit types and old style definitions

`f(y,x){...}` and `main(y){...}` leave out every type. Old C assumes `int` for
anything without a type, so this is legal when the warnings are turned off. `f` also
returns nothing, even though it is used in `y&&f(x,0)` as if it gave a value. `main`
takes only one parameter, which is the argument count under the name `y`, the same
name as used in `f` for something else. `putchar` is called with no header included,
which only works because old C guesses how a function is declared. `simple.c` gives
every function and parameter a type.

15. Single letter names and one line of code

All the names (`f`, `y`, `x`, `m`, `z`) are one letter, and the program is one line
with no spaces. Nothing tells a reader what each one is for, or where one part stops
and the next starts. `simple.c` uses descriptive names and normal formatting.
